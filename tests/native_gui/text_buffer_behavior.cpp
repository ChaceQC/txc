#include "stdlib/native_gui/core/text_buffer.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace tx::ui;

int main()
{
    text_buffer editor;
    editor.set_text(U"Aé👨‍👩‍👧‍👦🇨🇳");
    const auto original = editor.text();
    assert(editor.erase(true));
    assert(editor.text() == U"Aé👨‍👩‍👧‍👦");
    assert(editor.undo() && editor.text() == original);
    assert(editor.caret() == original.size() && editor.anchor() == original.size());
    assert(editor.redo() && editor.text() == U"Aé👨‍👩‍👧‍👦");
    editor.move(-1, false);
    assert(editor.caret() == 3);
    assert(editor.erase(true) && editor.text() == U"A👨‍👩‍👧‍👦");
    editor.select(1, editor.text().size());
    editor.insert(U"中文");
    assert(editor.text() == U"A中文");
    editor.read_only = true;
    assert(!editor.insert(U"错误") && !editor.erase(true) && !editor.undo());
    editor.read_only = false;
    editor.multiline = true;
    editor.set_text({});
    editor.insert(U"甲\r\n乙\r丙\n");
    assert(editor.text() == U"甲\n乙\n丙\n");
    editor.limit = editor.text().size();
    const auto before = editor.text();
    bool rejected = false;
    try
    {
        editor.insert(U"丁");
    }
    catch (const std::length_error&)
    {
        rejected = true;
    }
    assert(rejected && editor.text() == before);
    editor.limit = 100;
    editor.set_text(U"a");
    editor.insert(U"́");
    assert(editor.caret() == 2);
    editor.erase(true);
    assert(editor.text().empty());
    editor.set_text(U"first second");
    editor.move_word(-1, false);
    assert(editor.caret() == 6);
    editor.move_word(-1, false);
    assert(editor.caret() == 0);
    editor.move_word(1, false);
    assert(editor.caret() == 6);
    editor.erase_word(false);
    assert(editor.text() == U"first ");
    editor.undo();
    assert(editor.text() == U"first second" && editor.caret() == 6);
    editor.set_text(U"a\v\f\x85\u2028\u2029b");
    assert(editor.text() == U"a\n\n\n\n\nb");
    editor.multiline = false;
    editor.set_text(U"a\r\n\v\f\x85\u2028\u2029\tb");
    assert(editor.text() == U"ab");
    std::cout << "self text editor / grapheme navigation / undo / normalization PASS\n";
}
