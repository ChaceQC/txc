#include "stdlib/native_gui/data_model.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace tx::ui
{
data_model::data_model(data_kind kind, std::vector<data_column> columns) : kind_(kind), columns_(std::move(columns))
{
    if (kind == data_kind::table && (columns_.empty() || columns_.size() > 256))
    {
        throw std::invalid_argument("表格需要 1–256 列");
    }
    std::unordered_set<std::int64_t> ids;
    for (const auto& column : columns_)
    {
        if (column.id <= 0 || !ids.insert(column.id).second || !std::isfinite(column.width) ||
            column.width < 24 || column.width > 16384 || column.alignment < 0 || column.alignment > 2 ||
            column.title.size() > 16384)
        {
            throw std::invalid_argument("列 ID 重复或非法，或列宽/标题/对齐超出范围");
        }
    }
    column_order_.resize(columns_.size());
    std::iota(column_order_.begin(), column_order_.end(), 0);
}

const data_snapshot& data_model::snapshot() const noexcept
{
    return snapshot_;
}

const std::vector<data_column>& data_model::columns() const noexcept
{
    return columns_;
}

const std::vector<std::size_t>& data_model::column_order() const noexcept
{
    return column_order_;
}

data_kind data_model::kind() const noexcept
{
    return kind_;
}

std::int64_t data_model::revision() const noexcept
{
    return revision_;
}

void data_model::validate(data_snapshot& next) const
{
    if (next.rows.size() > 100000)
    {
        throw std::length_error("数据模型超过 100000 行");
    }
    std::size_t scalars = 0;
    for (std::size_t index = 0; index < next.rows.size(); ++index)
    {
        const auto& row = next.rows[index];
        if (row.id <= 0 || !next.indices.emplace(row.id, index).second ||
            (used_ids_.contains(row.id) && !snapshot_.indices.contains(row.id)))
        {
            throw std::invalid_argument("行 ID 非法、重复或已删除后复用");
        }
        if (row.cells.size() != (kind_ == data_kind::table ? columns_.size() : 1))
        {
            throw std::invalid_argument("行单元格数量与列定义不符");
        }
        for (const auto& cell : row.cells)
        {
            scalars += cell.size();
            if (scalars > 16 * 1024 * 1024 || cell.size() > 65536)
            {
                throw std::length_error("模型文本超过 64 MiB，或单元格超过 65536 个标量");
            }
        }
        if (kind_ == data_kind::tree)
        {
            next.children[row.parent].push_back(index);
        }
    }
    if (kind_ != data_kind::tree)
    {
        return;
    }
    for (const auto& row : next.rows)
    {
        auto parent = row.parent;
        unsigned depth = 1;
        while (parent != 0)
        {
            const auto found = next.indices.find(parent);
            if (found == next.indices.end() || parent == row.id || ++depth > 64)
            {
                throw std::invalid_argument("树的父项不存在、包含循环或深度超过 64");
            }
            parent = next.rows[found->second].parent;
        }
    }
}

void data_model::touch()
{
    if (revision_ == std::numeric_limits<std::int64_t>::max())
    {
        throw std::length_error("数据修订号耗尽");
    }
    ++revision_;
    request_ = 0;
}

void data_model::commit(data_snapshot next)
{
    validate(next);
    auto used = used_ids_;
    for (const auto& row : next.rows)
    {
        used.insert(row.id);
    }
    if (used.size() > 1000000)
    {
        throw std::length_error("视图生命周期行 ID 超过一百万");
    }
    touch();
    std::swap(snapshot_, next);
    used_ids_.swap(used);
}

void data_model::edit(std::vector<data_row> rows, data_edit operation)
{
    data_snapshot next;
    if (operation != data_edit::replace)
    {
        next.rows = snapshot_.rows;
    }
    if (operation == data_edit::update)
    {
        std::unordered_set<std::int64_t> ids;
        for (auto& row : rows)
        {
            const auto found = snapshot_.indices.find(row.id);
            if (found == snapshot_.indices.end() || !ids.insert(row.id).second)
            {
                throw std::invalid_argument("更新的行 ID 不存在或重复");
            }
            next.rows[found->second] = std::move(row);
        }
    }
    else
    {
        if (rows.size() > 100000 || next.rows.size() > 100000 - rows.size())
        {
            throw std::length_error("数据模型超过 100000 行");
        }
        next.rows.insert(next.rows.end(), std::make_move_iterator(rows.begin()), std::make_move_iterator(rows.end()));
    }
    commit(std::move(next));
}

void data_model::remove(const std::vector<std::int64_t>& ids)
{
    std::unordered_set<std::int64_t> removed;
    for (const auto id : ids)
    {
        if (!snapshot_.indices.contains(id) || !removed.insert(id).second)
        {
            throw std::invalid_argument("删除的行 ID 不存在或重复");
        }
    }
    data_snapshot next;
    for (const auto& row : snapshot_.rows)
    {
        bool erase = removed.contains(row.id);
        auto parent = kind_ == data_kind::tree ? row.parent : 0;
        while (!erase && parent)
        {
            erase = removed.contains(parent);
            parent = snapshot_.rows[snapshot_.indices.at(parent)].parent;
        }
        if (!erase)
        {
            next.rows.push_back(row);
        }
    }
    commit(std::move(next));
}

void data_model::reorder(const std::vector<std::int64_t>& ids)
{
    if (ids.size() != snapshot_.rows.size())
    {
        throw std::invalid_argument("排序必须包含每个现存 ID 且不重复");
    }
    data_snapshot next;
    for (const auto id : ids)
    {
        const auto found = snapshot_.indices.find(id);
        if (found == snapshot_.indices.end())
        {
            throw std::invalid_argument("排序包含不存在的行 ID");
        }
        next.rows.push_back(snapshot_.rows[found->second]);
    }
    commit(std::move(next));
}

std::int64_t data_model::begin_page()
{
    if (next_request_ == std::numeric_limits<std::int64_t>::max())
    {
        throw std::length_error("分页请求号耗尽");
    }
    request_ = next_request_++;
    return request_;
}

void data_model::apply_page(std::int64_t request, std::int64_t revision, std::vector<data_row> rows)
{
    if (!request || request != request_ || revision != revision_)
    {
        throw std::invalid_argument("分页请求或预期修订已过期");
    }
    edit(std::move(rows), data_edit::replace);
}
}
