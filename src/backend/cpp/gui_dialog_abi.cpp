#include "backend/cpp/graphics_result.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/gui/windows/dialogs.hpp"

#include <cctype>

using namespace tx_generated;
using namespace tx_generated::gui;

namespace
{

std::vector<file_filter> read_filters(const void* value)
{
    const auto& records = detail::vector_value<std::any>(value).data().values;
    if (records.size() > 64)
    {
        fail("resource_limit", "文件过滤器最多 64 组");
    }
    std::vector<file_filter> filters;
    for (const auto& item : records)
    {
        const auto& record = std::any_cast<const dynamic_struct&>(item);
        file_filter filter;
        filter.name = wide_text(std::any_cast<std::string>(record->read_field(0)));
        const auto list = std::any_cast<string_vector>(record->read_field(1));
        if (list.data().values.empty() || list.data().values.size() > 64)
        {
            fail("invalid_argument", "每组过滤器需要 1–64 个扩展名");
        }
        for (const auto& extension : list.data().values)
        {
            const auto& text = extension.get();
            if (text.empty() || text.size() > 64)
            {
                fail("invalid_argument", "扩展名长度非法");
            }
            if (text != "*")
            {
                for (const unsigned char ch : text)
                {
                    if (!std::isalnum(ch) && ch != '_' && ch != '-')
                    {
                        fail("invalid_argument", "扩展名只接受字母数字、下划线或连字符");
                    }
                }
            }
            if (!filter.pattern.empty())
            {
                filter.pattern += L";";
            }
            filter.pattern += text == "*" ? L"*.*" : L"*." + wide_text(text);
        }
        filters.push_back(std::move(filter));
    }
    return filters;
}

std::any paths_option(const std::optional<std::vector<std::string>>& paths, bool multiple)
{
    if (!multiple)
    {
        return graphics::option_value("option<str>", paths && !paths->empty() ? std::any(paths->front()) : std::any{});
    }
    if (!paths)
    {
        return graphics::option_value("option<vector<str>>");
    }
    string_vector items;
    for (const auto& path : *paths)
    {
        items.data().values.push_back(detail::vector_element_from_any<text_reference>(path));
    }
    items.data().refresh();
    return graphics::option_value("option<vector<str>>", std::move(items));
}

} // namespace

extern "C" int txrt_gui_open_file(graphics::resource* value, const void* title, const void* filters, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return paths_option(file_dialog(graphics::require_window(value), file_dialog_kind::open,
            detail::text_value(title), read_filters(filters)), false);
    });
}

extern "C" int txrt_gui_open_files(graphics::resource* value, const void* title, const void* filters, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return paths_option(file_dialog(graphics::require_window(value), file_dialog_kind::multiple,
            detail::text_value(title), read_filters(filters)), true);
    });
}

extern "C" int txrt_gui_save_file(graphics::resource* value, const void* title, const void* filters, const void* extension, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return paths_option(file_dialog(graphics::require_window(value), file_dialog_kind::save,
            detail::text_value(title), read_filters(filters), detail::text_value(extension)), false);
    });
}

extern "C" int txrt_gui_pick_folder(graphics::resource* value, const void* title, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return paths_option(file_dialog(graphics::require_window(value), file_dialog_kind::folder,
            detail::text_value(title), {}), false);
    });
}

extern "C" int txrt_gui_message_box(graphics::resource* value, const void* title, const void* text, const void* buttons, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return message_box(graphics::require_window(value), detail::text_value(title),
            detail::text_value(text), detail::text_value(buttons));
    });
}

extern "C" int txrt_gui_clipboard_text(graphics::resource* value, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        const auto text = clipboard_text(graphics::require_window(value));
        return graphics::option_value("option<str>", text ? std::any(*text) : std::any{});
    });
}

extern "C" int txrt_gui_set_clipboard_text(graphics::resource* value, const void* text, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        set_clipboard_text(graphics::require_window(value), detail::text_value(text));
        return {};
    });
}
