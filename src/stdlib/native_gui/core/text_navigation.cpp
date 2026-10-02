#include "stdlib/native_gui/core/text_buffer.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx::ui
{
bool text_buffer::erase_word(bool backward)
{
    if (read_only)
    {
        return false;
    }
    const auto old_anchor = anchor_, old_caret = caret_;
    if (anchor_ == caret_)
    {
        move_word(backward ? -1 : 1, true);
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

void text_buffer::move_word(int direction, bool extend)
{
    if (direction != -1 && direction != 1)
    {
        throw std::invalid_argument("光标移动方向必须为 -1 或 1");
    }
    auto position = caret_;
    if (direction < 0)
    {
        while (position && unicode_whitespace(text_[position - 1]))
        {
            --position;
        }
        auto boundary = std::lower_bound(words_.begin(), words_.end(), position);
        if (boundary != words_.begin())
        {
            --boundary;
        }
        caret_ = *boundary;
    }
    else
    {
        const auto boundary = std::upper_bound(words_.begin(), words_.end(), position);
        position = boundary == words_.end() ? text_.size() : *boundary;
        while (position < text_.size() && unicode_whitespace(text_[position]))
        {
            ++position;
        }
        caret_ = snap(position);
    }
    if (!extend)
    {
        anchor_ = caret_;
    }
}

void text_buffer::select_word(std::size_t index)
{
    if (index > text_.size())
    {
        throw std::out_of_range("文本索引越界");
    }
    if (text_.empty())
    {
        anchor_ = caret_ = 0;
        return;
    }
    index = std::min(index, text_.size() - 1);
    const auto end = std::upper_bound(words_.begin(), words_.end(), index);
    anchor_ = snap(*std::prev(end));
    caret_ = snap(*end);
}
}
