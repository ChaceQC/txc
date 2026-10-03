#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/platform/windows_window.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <uiautomationclient.h>
#include <future>
#include <iostream>

namespace ui = tx_generated::native_gui;

namespace
{
void check(HRESULT status)
{
    if (FAILED(status))
    {
        throw std::runtime_error("UIA HRESULT " + std::to_string(status));
    }
}

struct release_com
{
    template<class type>
    void operator()(type* value) const
    {
        if (value)
        {
            value->Release();
        }
    }
};
template<class type>
using com_ptr = std::unique_ptr<type, release_com>;

com_ptr<IUIAutomationElement> find(IUIAutomation& automation, IUIAutomationElement& root, const wchar_t* name)
{
    VARIANT text;
    VariantInit(&text);
    text.vt = VT_BSTR;
    text.bstrVal = SysAllocString(name);
    IUIAutomationCondition* raw_condition = nullptr;
    const auto status = automation.CreatePropertyCondition(UIA_NamePropertyId, text, &raw_condition);
    VariantClear(&text);
    check(status);
    com_ptr<IUIAutomationCondition> condition(raw_condition);
    IUIAutomationElement* raw = nullptr;
    check(root.FindFirst(TreeScope_Descendants, condition.get(), &raw));
    if (!raw)
    {
        throw std::runtime_error("UIA cannot find semantic child");
    }
    return com_ptr<IUIAutomationElement>(raw);
}

void client(HWND hwnd)
{
    check(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    IUIAutomation* raw = nullptr;
    check(CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER,
        __uuidof(IUIAutomation), reinterpret_cast<void**>(&raw)));
    com_ptr<IUIAutomation> automation(raw);
    IUIAutomationElement* raw_root = nullptr;
    check(automation->ElementFromHandle(hwnd, &raw_root));
    com_ptr<IUIAutomationElement> root(raw_root);
    auto button = find(*automation, *root, L"执行命令");
    IUIAutomationInvokePattern* invoke_raw = nullptr;
    check(button->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), reinterpret_cast<void**>(&invoke_raw)));
    com_ptr<IUIAutomationInvokePattern> invoke(invoke_raw);
    check(invoke->Invoke());
    auto editor = find(*automation, *root, L"输入内容");
    IUIAutomationValuePattern* value_raw = nullptr;
    check(editor->GetCurrentPatternAs(UIA_ValuePatternId, __uuidof(IUIAutomationValuePattern), reinterpret_cast<void**>(&value_raw)));
    com_ptr<IUIAutomationValuePattern> value(value_raw);
    check(value->SetValue(L"UIA 修改😀"));
    IUIAutomationTextPattern* text_raw = nullptr;
    check(editor->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), reinterpret_cast<void**>(&text_raw)));
    com_ptr<IUIAutomationTextPattern> text(text_raw);
    IUIAutomationTextRange* range_raw = nullptr;
    check(text->get_DocumentRange(&range_raw));
    com_ptr<IUIAutomationTextRange> range(range_raw);
    BSTR content = nullptr;
    check(range->GetText(-1, &content));
    const bool valid = std::wstring_view(content, SysStringLen(content)) == L"UIA 修改😀";
    SysFreeString(content);
    if (!valid)
    {
        throw std::runtime_error("UIA text range mismatch");
    }
    check(range->Select());
    auto slider = find(*automation, *root, L"音量");
    IUIAutomationRangeValuePattern* range_value_raw = nullptr;
    check(slider->GetCurrentPatternAs(UIA_RangeValuePatternId, __uuidof(IUIAutomationRangeValuePattern), reinterpret_cast<void**>(&range_value_raw)));
    com_ptr<IUIAutomationRangeValuePattern> range_value(range_value_raw);
    check(range_value->SetValue(72));
}
}

int main(int argc, char** argv)
{
    try
    {
        tx_generated::detail::current_runtime_context().main_thread = true;
        auto app = ui::open_app(argc > 1 ? argv[1] : "");
        auto window = ui::create_window(*app, "UIA protocol check", 600, 360);
        auto root = ui::root(*window);
        auto command = ui::create_command(*window, "执行命令", "", true);
        auto button = ui::create(*root, tx::graphics_kind::native_button, "执行命令");
        ui::bind_command(*button, *command);
        auto editor = ui::create(*root, tx::graphics_kind::native_text_box, "初始文本");
        ui::set_accessibility(*editor, "输入内容", "");
        auto slider = ui::create(*root, tx::graphics_kind::native_slider, "");
        ui::set_accessibility(*slider, "音量", "");
        window->visible = true;
        window->host->show();
        const auto hwnd = static_cast<tx::ui::windows_window&>(*window->host).hwnd;
        auto result = std::async(std::launch::async, [hwnd]
        {
            client(hwnd);
        });
        while (result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        {
            ui::next_event(*app, 20);
        }
        result.get();
        if (!command->checked || editor->editor->text() != U"UIA 修改😀" || slider->value != 72)
        {
            throw std::runtime_error("UIA did not operate actual controls");
        }
        ui::close_app(*app);
        std::cout << "UIA system client traversal / Invoke / Value / Text / Range PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
