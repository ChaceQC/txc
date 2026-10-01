#include "stdlib/gui/windows/models.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::gui
{
namespace
{

struct suppress_notifications
{
    explicit suppress_notifications(node& value) : state(value)
    {
        ++state.suppress;
        SendMessageW(state.hwnd, WM_SETREDRAW, FALSE, 0);
    }
    ~suppress_notifications()
    {
        SendMessageW(state.hwnd, WM_SETREDRAW, TRUE, 0);
        --state.suppress;
        InvalidateRect(state.hwnd, nullptr, FALSE);
    }
    node& state;
};

void select_native(node& view)
{
    if (view.kind == tx::graphics_kind::tree_view)
    {
        TreeView_SelectItem(view.hwnd, view.selection.empty() ? nullptr : view.tree_handles.at(view.selection.front()));
        return;
    }
    ListView_SetItemState(view.hwnd, -1, 0, LVIS_SELECTED);
    for (const auto id : view.selection)
    {
        const auto index = view.model->snapshot->indices.at(id);
        ListView_SetItemState(view.hwnd, static_cast<int>(index), LVIS_SELECTED, LVIS_SELECTED);
    }
}

void refresh_tree(node& view)
{
    std::vector<std::int64_t> expanded;
    for (const auto& [id, handle] : view.tree_handles)
    {
        if (TreeView_GetItemState(view.hwnd, handle, TVIS_EXPANDED) & TVIS_EXPANDED)
        {
            expanded.push_back(id);
        }
    }
    TreeView_DeleteAllItems(view.hwnd);
    view.tree_handles.clear();
    const auto snapshot = view.model->snapshot;
    for (unsigned pass = 0; view.tree_handles.size() < snapshot->items.size() && pass < depth_limit; ++pass)
    {
        for (const auto& item : snapshot->items)
        {
            if (view.tree_handles.contains(item.id) || (item.parent && !view.tree_handles.contains(*item.parent)))
            {
                continue;
            }
            TVINSERTSTRUCTW insert{};
            insert.hParent = item.parent ? view.tree_handles.at(*item.parent) : TVI_ROOT;
            insert.hInsertAfter = TVI_LAST;
            insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
            insert.item.pszText = const_cast<wchar_t*>(item.cells.front().c_str());
            insert.item.lParam = static_cast<LPARAM>(item.id);
            insert.item.cChildren = item.load_state == "loaded" ? 0 : 1;
            const auto handle = reinterpret_cast<HTREEITEM>(SendMessageW(view.hwnd, TVM_INSERTITEMW, 0,
                reinterpret_cast<LPARAM>(&insert)));
            if (!handle)
            {
                platform_fail(view.model->owner.lock().get(), "实例化树节点", GetLastError());
            }
            view.tree_handles.emplace(item.id, handle);
        }
    }
    for (const auto& item : snapshot->items)
    {
        if (item.parent)
        {
            TVITEMW parent{};
            parent.mask = TVIF_CHILDREN;
            parent.hItem = view.tree_handles.at(*item.parent);
            parent.cChildren = 1;
            SendMessageW(view.hwnd, TVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&parent));
        }
    }
    for (const auto id : expanded)
    {
        if (const auto found = view.tree_handles.find(id); found != view.tree_handles.end())
        {
            TreeView_Expand(view.hwnd, found->second, TVE_EXPAND);
        }
    }
}

} // namespace

void view_dpi_changed(node& view)
{
    if (!view.model || view.kind != tx::graphics_kind::table_view)
    {
        return;
    }
    const auto scale = owner_window(view).dpi / 96.0;
    for (std::size_t index = 0; index < view.model->columns.size(); ++index)
    {
        ListView_SetColumnWidth(view.hwnd, static_cast<int>(index),
            static_cast<int>(std::lround(view.model->columns[index].width * scale)));
    }
}

std::shared_ptr<node> create_view(node& parent, data_model& model, tx::graphics_kind kind, bool multiple)
{
    if (parent.window.lock() != model.hosted_window.lock())
    {
        fail("wrong_owner", "模型与视图必须属于同一窗口");
    }
    if (kind == tx::graphics_kind::tree_view && multiple)
    {
        fail("invalid_argument", "原生树视图目前只接受单选模式");
    }
    auto result = create(parent, kind, {}, multiple);
    result->multiple = multiple;
    result->height = {length_mode::stretch, 1};
    result->minimum = {60, 40};
    result->model = model.shared_from_this();
    model.views.push_back(result);
    try
    {
        if (kind != tx::graphics_kind::tree_view)
        {
            ListView_SetExtendedListViewStyle(result->hwnd, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        }
        if (kind == tx::graphics_kind::table_view)
        {
            for (std::size_t index = 0; index < model.columns.size(); ++index)
            {
                const auto& column = model.columns[index];
                LVCOLUMNW native{};
                native.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
                native.pszText = const_cast<wchar_t*>(column.title.c_str());
                native.cx = static_cast<int>(std::lround(column.width * owner_window(parent).dpi / 96.0));
                native.fmt = column.alignment;
                if (SendMessageW(result->hwnd, LVM_INSERTCOLUMNW, index, reinterpret_cast<LPARAM>(&native)) == -1)
                {
                    platform_fail(model.owner.lock().get(), "创建表格列", GetLastError());
                }
            }
        }
        refresh_view(*result, false);
    }
    catch (...)
    {
        close(*result);
        throw;
    }
    return result;
}

void refresh_view(node& view, bool notify_selection)
{
    const auto old = view.selection;
    {
        suppress_notifications suppress(view);
        std::erase_if(view.selection, [&](const auto id)
        {
            return !view.model->snapshot->indices.contains(id);
        });
        if (view.kind == tx::graphics_kind::tree_view)
        {
            refresh_tree(view);
        }
        else
        {
            if (!SendMessageW(view.hwnd, LVM_SETITEMCOUNT, view.model->snapshot->items.size(), LVSICF_NOSCROLL))
            {
                platform_fail(view.model->owner.lock().get(), "更新虚拟列表长度", GetLastError());
            }
        }
        select_native(view);
    }
    if (notify_selection && old != view.selection)
    {
        ++view.revision;
        notify(view, "selection_changed");
    }
}

void set_selected_ids(node& view, const std::vector<std::int64_t>& ids)
{
    require_idle(view);
    if (!view.multiple && ids.size() > 1)
    {
        fail("invalid_argument", "单选视图最多选择一项");
    }
    std::unordered_set<std::int64_t> checked;
    for (const auto id : ids)
    {
        if (!view.model->snapshot->indices.contains(id) || !checked.insert(id).second)
        {
            fail("invalid_argument", "选择 ID 不存在或重复");
        }
    }
    suppress_notifications suppress(view);
    view.selection = ids;
    select_native(view);
    ++view.revision;
}

} // namespace tx_generated::gui
