#pragma once

#include "stdlib/native_gui/core/unicode.hpp"

#include <deque>

namespace tx::ui
{
class text_buffer
{
public:
    const std::u32string& text() const noexcept;
    std::size_t caret() const noexcept;
    std::size_t anchor() const noexcept;
    std::pair<std::size_t, std::size_t> selection() const noexcept;
    void set_text(std::u32string value);
    void select(std::size_t anchor, std::size_t caret);
    void move(int direction, bool extend);
    void move_word(int direction, bool extend);
    void select_word(std::size_t index);
    bool insert(std::u32string_view value);
    bool erase(bool backward);
    bool erase_word(bool backward);
    bool undo();
    bool redo();
    bool read_only = false;
    bool multiline = false;
    std::size_t limit = 1024 * 1024;
private:
    struct edit
    {
        std::size_t position, old_anchor, old_caret;
        std::u32string removed, inserted;
    };
    std::u32string text_;
    std::vector<std::size_t> boundaries_{0};
    std::vector<std::size_t> words_{0};
    std::size_t caret_ = 0, anchor_ = 0;
    std::deque<edit> undo_, redo_;
    std::size_t snap(std::size_t offset) const;
};
}
