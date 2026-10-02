#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace tx::ui
{
enum class data_kind
{
    list, table, tree
};

enum class data_edit
{
    replace, append, update
};

struct data_row
{
    std::int64_t id = 0, parent = 0;
    std::vector<std::u32string> cells;
    bool has_children = false;
};

struct data_column
{
    std::int64_t id = 0;
    std::u32string title;
    double width = 120;
    int alignment = 0;
    bool visible = true;
};

struct data_snapshot
{
    std::vector<data_row> rows;
    std::unordered_map<std::int64_t, std::size_t> indices;
    std::unordered_map<std::int64_t, std::vector<std::size_t>> children;
};

class data_model
{
public:
    explicit data_model(data_kind kind, std::vector<data_column> columns = {});
    const data_snapshot& snapshot() const noexcept;
    const std::vector<data_column>& columns() const noexcept;
    const std::vector<std::size_t>& column_order() const noexcept;
    data_kind kind() const noexcept;
    std::int64_t revision() const noexcept;
    void edit(std::vector<data_row> rows, data_edit operation);
    void remove(const std::vector<std::int64_t>& ids);
    void reorder(const std::vector<std::int64_t>& ids);
    void set_column_width(std::int64_t id, double width);
    void set_column_visible(std::int64_t id, bool visible);
    void set_column_order(const std::vector<std::int64_t>& ids);
    std::int64_t begin_page();
    void apply_page(std::int64_t request, std::int64_t revision, std::vector<data_row> rows);
private:
    data_kind kind_;
    data_snapshot snapshot_;
    std::vector<data_column> columns_;
    std::vector<std::size_t> column_order_;
    std::unordered_set<std::int64_t> used_ids_;
    std::int64_t revision_ = 0, request_ = 0, next_request_ = 1;
    void validate(data_snapshot& next) const;
    void commit(data_snapshot next);
    void touch();
    std::size_t column_index(std::int64_t id) const;
};
}
