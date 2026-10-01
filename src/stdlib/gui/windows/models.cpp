#include "stdlib/gui/windows/models.hpp"

#include <algorithm>

namespace tx_generated::gui
{

data_model& require_model(graphics::resource* value)
{
    auto& result = graphics::asset<data_model>(value);
    const auto window = result.hosted_window.lock();
    if (!window || window->closed)
    {
        fail("closed_resource", "数据模型所属窗口已关闭");
    }
    require_system_idle(*window);
    return result;
}

std::shared_ptr<data_model> create_model(graphics::window& window, tx::graphics_kind kind, std::vector<data_column> columns)
{
    require_system_idle(window);
    if (kind == tx::graphics_kind::table_model && (columns.empty() || columns.size() > 256))
    {
        fail("invalid_argument", "表格需要 1–256 列");
    }
    std::unordered_set<std::int64_t> ids;
    for (const auto& column : columns)
    {
        if (column.id < 0 || !ids.insert(column.id).second)
        {
            fail("duplicate_id", "表格列 ID 非法或重复");
        }
        checked_dimension(column.width);
    }
    auto result = graphics::own<data_model>(*window.owner.lock());
    result->kind = kind;
    result->hosted_window = window.shared_from_this();
    result->columns = std::move(columns);
    return result;
}

void validate_model(const data_model& model, model_snapshot& next)
{
    const bool tree = model.kind == tx::graphics_kind::tree_model;
    if (next.items.size() > (tree ? 10000 : 100000))
    {
        fail("resource_limit", "数据模型超过项数限制");
    }
    next.indices.clear();
    std::size_t bytes = 0;
    for (std::size_t index = 0; index < next.items.size(); ++index)
    {
        const auto& item = next.items[index];
        if (item.id < 0 || !next.indices.emplace(item.id, index).second ||
            (model.used_ids.contains(item.id) && !model.snapshot->indices.contains(item.id)))
        {
            fail("duplicate_id", "模型项 ID 非法、重复或已删除后复用");
        }
        const auto columns = model.kind == tx::graphics_kind::table_model ? model.columns.size() : 1;
        if (item.cells.size() != columns)
        {
            fail("invalid_argument", "模型项的文本列数不匹配");
        }
        for (const auto& text : item.cells)
        {
            bytes += text.size() * sizeof(wchar_t);
            if (bytes > 64 * 1024 * 1024)
            {
                fail("resource_limit", "模型文本超过 64 MiB");
            }
        }
        if (tree && item.load_state != "unloaded" && item.load_state != "loading" &&
            item.load_state != "loaded" && item.load_state != "failed")
        {
            fail("invalid_argument", "树载入状态必须为 unloaded/loading/loaded/failed");
        }
    }
    if (!tree)
    {
        return;
    }
    for (const auto& item : next.items)
    {
        auto parent = item.parent;
        unsigned depth = 0;
        while (parent)
        {
            const auto found = next.indices.find(*parent);
            if (found == next.indices.end())
            {
                fail("invalid_argument", "树节点的父 ID 不存在");
            }
            if (*parent == item.id || ++depth >= depth_limit)
            {
                fail("invalid_argument", "树存在环或超过 64 层");
            }
            parent = next.items[found->second].parent;
        }
    }
}

void commit_model(data_model& model, std::shared_ptr<model_snapshot> next)
{
    validate_model(model, *next);
    auto ids = model.used_ids;
    for (const auto& item : next->items)
    {
        ids.insert(item.id);
    }
    if (ids.size() > 1000000)
    {
        fail("resource_limit", "模型生命周期 ID 总数超过 1000000");
    }
    const auto old = model.snapshot;
    const auto old_revision = model.revision;
    std::vector<std::pair<std::shared_ptr<node>, std::vector<std::int64_t>>> views;
    for (const auto& weak : model.views)
    {
        if (auto view = weak.lock(); view && !view->closed)
        {
            views.emplace_back(view, view->selection);
        }
    }
    model.snapshot = std::move(next);
    ++model.revision;
    try
    {
        for (const auto& [view, selection] : views)
        {
            refresh_view(*view, false);
        }
    }
    catch (...)
    {
        model.snapshot = old;
        model.revision = old_revision;
        for (const auto& [view, selection] : views)
        {
            view->selection = selection;
            try
            {
                refresh_view(*view, false);
            }
            catch (...)
            {
            }
        }
        throw;
    }
    model.used_ids.swap(ids);
    model.request_id = 0;
    for (const auto& [view, selection] : views)
    {
        if (view->selection != selection)
        {
            ++view->revision;
            notify(*view, "selection_changed");
        }
    }
}

void edit_model(data_model& model, std::vector<data_item> items, model_edit edit)
{
    auto next = std::make_shared<model_snapshot>();
    if (edit != model_edit::replace)
    {
        next->items = model.snapshot->items;
    }
    if (edit == model_edit::update)
    {
        std::unordered_set<std::int64_t> ids;
        for (auto& item : items)
        {
            const auto found = model.snapshot->indices.find(item.id);
            if (found == model.snapshot->indices.end() || !ids.insert(item.id).second)
            {
                fail("invalid_argument", "更新项 ID 不存在或重复");
            }
            next->items[found->second] = std::move(item);
        }
    }
    else
    {
        next->items.insert(next->items.end(), std::make_move_iterator(items.begin()), std::make_move_iterator(items.end()));
    }
    commit_model(model, std::move(next));
}

void remove_items(data_model& model, const std::vector<std::int64_t>& ids)
{
    std::unordered_set<std::int64_t> removed;
    for (const auto id : ids)
    {
        if (!model.snapshot->indices.contains(id) || !removed.insert(id).second)
        {
            fail("invalid_argument", "删除项 ID 不存在或重复");
        }
    }
    auto next = std::make_shared<model_snapshot>();
    for (const auto& item : model.snapshot->items)
    {
        bool remove = removed.contains(item.id);
        auto parent = item.parent;
        while (!remove && parent)
        {
            remove = removed.contains(*parent);
            parent = model.snapshot->items[model.snapshot->indices.at(*parent)].parent;
        }
        if (!remove)
        {
            next->items.push_back(item);
        }
    }
    commit_model(model, std::move(next));
}

void set_order(data_model& model, const std::vector<std::int64_t>& ids)
{
    if (ids.size() != model.snapshot->items.size())
    {
        fail("invalid_argument", "排序必须包含全部现存 ID");
    }
    auto next = std::make_shared<model_snapshot>();
    for (const auto id : ids)
    {
        const auto found = model.snapshot->indices.find(id);
        if (found == model.snapshot->indices.end())
        {
            fail("invalid_argument", "排序 ID 不存在");
        }
        next->items.push_back(model.snapshot->items[found->second]);
    }
    commit_model(model, std::move(next));
}

std::int64_t begin_page(data_model& model)
{
    model.request_id = model.next_request_id++;
    return model.request_id;
}

void apply_page(data_model& model, std::int64_t request, std::int64_t revision, std::vector<data_item> items)
{
    if (!request || request != model.request_id || revision != model.revision)
    {
        fail("stale_revision", "分页请求已过期");
    }
    edit_model(model, std::move(items), model_edit::replace);
}

void close_model(data_model& model)
{
    for (const auto& weak : model.views)
    {
        if (const auto view = weak.lock(); view && !view->closed)
        {
            fail("model_in_use", "模型仍被视图使用，请先关闭视图");
        }
    }
    graphics::close_owned(model);
}

void data_model::release_native() noexcept
{
    snapshot.reset();
    columns.clear();
    used_ids.clear();
    views.clear();
}

} // namespace tx_generated::gui
