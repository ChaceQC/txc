#pragma once

#include "stdlib/gui/windows/commands.hpp"
#include <unordered_map>
#include <unordered_set>

namespace tx_generated::gui
{

struct data_item
{
    std::int64_t id = 0;
    std::optional<std::int64_t> parent;
    std::vector<std::wstring> cells;
    std::string load_state = "loaded";
};

struct data_column
{
    std::int64_t id = 0;
    std::wstring title;
    double width = 120;
    int alignment = LVCFMT_LEFT;
};

struct model_snapshot
{
    std::vector<data_item> items;
    std::unordered_map<std::int64_t, std::size_t> indices;
};

struct data_model : graphics::owned_resource, std::enable_shared_from_this<data_model>
{
    explicit data_model(tx::graphics_kind kind = tx::graphics_kind::list_model) : owned_resource(kind)
    {
    }
    std::shared_ptr<const model_snapshot> snapshot = std::make_shared<model_snapshot>();
    std::vector<data_column> columns;
    std::unordered_set<std::int64_t> used_ids;
    std::vector<std::weak_ptr<node>> views;
    std::int64_t revision = 0, request_id = 0, next_request_id = 1;
    void release_native() noexcept override;
};

enum class model_edit
{
    replace, append, update
};

data_model& require_model(graphics::resource* value);
std::shared_ptr<data_model> create_model(graphics::window& window, tx::graphics_kind kind, std::vector<data_column> columns = {});
void validate_model(const data_model& model, model_snapshot& next);
void commit_model(data_model& model, std::shared_ptr<model_snapshot> next);
void edit_model(data_model& model, std::vector<data_item> items, model_edit edit);
void remove_items(data_model& model, const std::vector<std::int64_t>& ids);
void set_order(data_model& model, const std::vector<std::int64_t>& ids);
std::int64_t begin_page(data_model& model);
void apply_page(data_model& model, std::int64_t request, std::int64_t revision, std::vector<data_item> items);
void close_model(data_model& model);
std::shared_ptr<node> create_view(node& parent, data_model& model, tx::graphics_kind kind, bool multiple);
void refresh_view(node& view, bool notify_selection);
void set_selected_ids(node& view, const std::vector<std::int64_t>& ids);
LRESULT data_notification(NMHDR& header);
void view_dpi_changed(node& view);

} // namespace tx_generated::gui
