#include "stdlib/native_gui/core/text_buffer.hpp"
#include "stdlib/native_gui/core/line_break.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx::ui
{
namespace
{
std::u32string normalize(std::u32string_view value, bool multiline)
{
    std::u32string result;
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        auto scalar = value[index];
        if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
        {
            throw std::invalid_argument("文本包含非法 Unicode 标量");
        }
        if (scalar == U'\r')
        {
            scalar = U'\n';
            if (index + 1 < value.size() && value[index + 1] == U'\n')
            {
                ++index;
            }
        }
        else if (hard_line_break(scalar))
        {
            scalar = U'\n';
        }
        if (scalar != U'\0' && (multiline || (scalar != U'\n' && scalar != U'\t')))
        {
            result.push_back(scalar);
        }
    }
    return result;
}
}

const std::u32string& text_buffer::text() const noexcept
{
    return text_;
}

std::size_t text_buffer::caret() const noexcept
{
    return caret_;
}

std::size_t text_buffer::anchor() const noexcept
{
    return anchor_;
}

std::pair<std::size_t, std::size_t> text_buffer::selection() const noexcept
{
    return std::minmax(anchor_, caret_);
}

std::size_t text_buffer::snap(std::size_t offset) const
{
    if (offset > text_.size())
    {
        throw std::out_of_range("文本选区超过 Unicode 标量长度");
    }
    const auto found = std::upper_bound(boundaries_.begin(), boundaries_.end(), offset);
    return *std::prev(found);
}

void text_buffer::set_text(std::u32string value)
{
    value = normalize(value, multiline);
    if (value.size() > limit)
    {
        throw std::length_error("文本超过长度上限");
    }
    auto boundaries = grapheme_boundaries(value);
    auto words = word_boundaries(value);
    text_.swap(value);
    boundaries_.swap(boundaries);
    words_.swap(words);
    anchor_ = caret_ = text_.size();
    undo_.clear();
    redo_.clear();
}

void text_buffer::select(std::size_t anchor, std::size_t caret)
{
    const auto new_anchor = snap(anchor), new_caret = snap(caret);
    anchor_ = new_anchor;
    caret_ = new_caret;
}

void text_buffer::move(int direction, bool extend)
{
    if (direction != -1 && direction != 1)
    {
        throw std::invalid_argument("光标移动方向必须为 -1 或 1");
    }
    if (!extend && anchor_ != caret_)
    {
        caret_ = direction < 0 ? selection().first : selection().second;
    }
    else
    {
        const auto current = std::lower_bound(boundaries_.begin(), boundaries_.end(), caret_);
        if (direction < 0 && current != boundaries_.begin())
        {
            caret_ = *std::prev(current);
        }
        else if (direction > 0 && std::next(current) != boundaries_.end())
        {
            caret_ = *std::next(current);
        }
    }
    if (!extend)
    {
        anchor_ = caret_;
    }
}

bool text_buffer::insert(std::u32string_view value)
{
    if (read_only)
    {
        return false;
    }
    const auto replacement = normalize(value, multiline);
    const auto [begin, end] = selection();
    if (replacement.size() > limit || text_.size() - (end - begin) > limit - replacement.size())
    {
        throw std::length_error("文本超过长度上限");
    }
    if (begin == end && replacement.empty())
    {
        return false;
    }
    edit change{begin, anchor_, caret_, text_.substr(begin, end - begin), replacement};
    auto new_text = text_;
    new_text.replace(begin, end - begin, replacement);
    auto boundaries = grapheme_boundaries(new_text);
    auto words = word_boundaries(new_text);
    undo_.push_back(std::move(change));
    if (undo_.size() > 64)
    {
        undo_.pop_front();
    }
    redo_.clear();
    text_.swap(new_text);
    boundaries_.swap(boundaries);
    words_.swap(words);
    // 插入组合标记可能合并邻接字素，插入点向后对齐到合法边界。
    caret_ = *std::lower_bound(boundaries_.begin(), boundaries_.end(), begin + replacement.size());
    anchor_ = caret_;
    return true;
}

bool text_buffer::erase(bool backward)
{
    if (read_only)
    {
        return false;
    }
    const auto old_anchor = anchor_, old_caret = caret_;
    if (anchor_ == caret_)
    {
        move(backward ? -1 : 1, true);
        if (old_caret == caret_)
        {
            return false;
        }
    }
    try
    {
        const bool changed = insert({});
        if (changed)
        {
            undo_.back().old_anchor = old_anchor;
            undo_.back().old_caret = old_caret;
        }
        return changed;
    }
    catch (...)
    {
        anchor_ = old_anchor;
        caret_ = old_caret;
        throw;
    }
}

bool text_buffer::undo()
{
    if (read_only || undo_.empty())
    {
        return false;
    }
    const auto& change = undo_.back();
    auto replacement = text_;
    replacement.replace(change.position, change.inserted.size(), change.removed);
    auto boundaries = grapheme_boundaries(replacement);
    auto words = word_boundaries(replacement);
    redo_.push_back(change);
    anchor_ = change.old_anchor;
    caret_ = change.old_caret;
    text_.swap(replacement);
    boundaries_.swap(boundaries);
    words_.swap(words);
    undo_.pop_back();
    return true;
}

bool text_buffer::redo()
{
    if (read_only || redo_.empty())
    {
        return false;
    }
    const auto& change = redo_.back();
    auto replacement = text_;
    replacement.replace(change.position, change.removed.size(), change.inserted);
    auto boundaries = grapheme_boundaries(replacement);
    auto words = word_boundaries(replacement);
    undo_.push_back(change);
    const auto target = change.position + change.inserted.size();
    caret_ = *std::lower_bound(boundaries.begin(), boundaries.end(), target);
    anchor_ = caret_;
    text_.swap(replacement);
    boundaries_.swap(boundaries);
    words_.swap(words);
    redo_.pop_back();
    return true;
}
}
