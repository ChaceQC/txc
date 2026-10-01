#include "stdlib/gui/windows/models.hpp"

namespace tx_generated::gui
{
namespace
{

void selected(node& view, std::vector<std::int64_t> next)
{
    if (next == view.selection)
    {
        return;
    }
    view.selection = std::move(next);
    ++view.revision;
    graphics::event::control_data data;
    data.source_id = view.id;
    data.action = "selection_changed";
    data.revision = view.revision;
    if (view.selection.size() == 1)
    {
        data.item_id = view.selection.front();
    }
    graphics::enqueue_control(owner_window(view), std::move(data));
}

void expand(node& view, NMTREEVIEWW& notification)
{
    if (!(notification.action & TVE_EXPAND))
    {
        return;
    }
    const auto id = static_cast<std::int64_t>(notification.itemNew.lParam);
    auto& model = *view.model;
    const auto found = model.snapshot->indices.find(id);
    if (found == model.snapshot->indices.end())
    {
        return;
    }
    const auto& item = model.snapshot->items[found->second];
    if (item.load_state != "unloaded" && item.load_state != "failed")
    {
        return;
    }
    auto next = std::make_shared<model_snapshot>(*model.snapshot);
    next->items[found->second].load_state = "loading";
    model.snapshot = std::move(next);
    ++model.revision;
    model.request_id = 0;
    TVINSERTSTRUCTW placeholder{};
    placeholder.hParent = notification.itemNew.hItem;
    placeholder.hInsertAfter = TVI_LAST;
    placeholder.item.mask = TVIF_TEXT | TVIF_PARAM;
    placeholder.item.pszText = const_cast<wchar_t*>(L"正在载入…");
    placeholder.item.lParam = -1;
    SendMessageW(view.hwnd, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&placeholder));
    graphics::event::control_data data;
    data.source_id = view.id;
    data.action = "expand_requested";
    data.item_id = id;
    data.revision = model.revision;
    graphics::enqueue_control(owner_window(view), std::move(data));
}

} // namespace

LRESULT data_notification(NMHDR& header)
{
    DWORD_PTR reference = 0;
    if (!GetWindowSubclass(header.hwndFrom, control_proc, 1, &reference))
    {
        return 0;
    }
    auto& view = *reinterpret_cast<node*>(reference);
    if (!view.model || view.closed || view.model->closed)
    {
        return 0;
    }
    const auto snapshot = view.model->snapshot;
    if (header.code == LVN_GETDISPINFOW)
    {
        auto& info = reinterpret_cast<NMLVDISPINFOW&>(header);
        if (info.item.iItem >= 0 && static_cast<std::size_t>(info.item.iItem) < snapshot->items.size() &&
            info.item.iSubItem >= 0)
        {
            const auto& row = snapshot->items[info.item.iItem];
            if ((info.item.mask & LVIF_TEXT) && static_cast<std::size_t>(info.item.iSubItem) < row.cells.size())
            {
                view.notification_text = row.cells[info.item.iSubItem];
                info.item.pszText = view.notification_text.data();
            }
        }
        return 0;
    }
    if (view.suppress)
    {
        return 0;
    }
    if (header.code == LVN_ITEMCHANGED || header.code == LVN_ODSTATECHANGED)
    {
        std::vector<std::int64_t> ids;
        for (int index = ListView_GetNextItem(view.hwnd, -1, LVNI_SELECTED); index >= 0;
            index = ListView_GetNextItem(view.hwnd, index, LVNI_SELECTED))
        {
            if (static_cast<std::size_t>(index) < snapshot->items.size())
            {
                ids.push_back(snapshot->items[index].id);
            }
        }
        selected(view, std::move(ids));
    }
    else if (header.code == LVN_COLUMNCLICK)
    {
        const auto& click = reinterpret_cast<NMLISTVIEW&>(header);
        if (click.iSubItem >= 0 && static_cast<std::size_t>(click.iSubItem) < view.model->columns.size())
        {
            graphics::event::control_data data;
            data.source_id = view.id;
            data.action = "sort_requested";
            data.item_id = view.model->columns[click.iSubItem].id;
            data.revision = view.model->revision;
            graphics::enqueue_control(owner_window(view), std::move(data));
        }
    }
    else if (header.code == TVN_SELCHANGEDW)
    {
        const auto id = static_cast<std::int64_t>(reinterpret_cast<NMTREEVIEWW&>(header).itemNew.lParam);
        selected(view, snapshot->indices.contains(id) ? std::vector<std::int64_t>{id} : std::vector<std::int64_t>{});
    }
    else if (header.code == TVN_ITEMEXPANDINGW)
    {
        expand(view, reinterpret_cast<NMTREEVIEWW&>(header));
    }
    return 0;
}

} // namespace tx_generated::gui
