#include "stdlib/gui/windows/dialogs.hpp"
#include <shobjidl.h>

namespace tx_generated::gui
{
namespace
{

struct owner_guard
{
    explicit owner_guard(HWND window) : hwnd(window), enabled(IsWindowEnabled(window))
    {
    }
    ~owner_guard()
    {
        if (IsWindow(hwnd))
        {
            EnableWindow(hwnd, enabled);
        }
    }
    HWND hwnd;
    BOOL enabled;
};

std::string item_path(graphics::app& app, IShellItem& item)
{
    PWSTR raw = nullptr;
    graphics::check_hr(&app, item.GetDisplayName(SIGDN_FILESYSPATH, &raw), "读取文件系统路径");
    struct free_text
    {
        void operator()(wchar_t* value) const noexcept
        {
            CoTaskMemFree(value);
        }
    };
    std::unique_ptr<wchar_t, free_text> text(raw);
    return graphics::utf8(raw);
}

} // namespace

std::optional<std::vector<std::string>> file_dialog(graphics::window& window, file_dialog_kind kind,
    const std::string& title, const std::vector<file_filter>& filters, const std::string& extension)
{
    require_system_idle(window);
    const auto app = window.owner.lock();
    IFileDialog* raw = nullptr;
    const auto class_id = kind == file_dialog_kind::save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    graphics::check_hr(app.get(), CoCreateInstance(class_id, nullptr, CLSCTX_INPROC_SERVER,
        IID_IFileDialog, reinterpret_cast<void**>(&raw)), "创建文件对话框");
    graphics::com_ptr<IFileDialog> dialog(raw);
    DWORD options = 0;
    graphics::check_hr(app.get(), dialog->GetOptions(&options), "读取文件对话框选项");
    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
    if (kind == file_dialog_kind::save)
    {
        options |= FOS_OVERWRITEPROMPT;
    }
    else if (kind == file_dialog_kind::folder)
    {
        options |= FOS_PICKFOLDERS;
    }
    else
    {
        options |= FOS_FILEMUSTEXIST;
    }
    if (kind == file_dialog_kind::multiple)
    {
        options |= FOS_ALLOWMULTISELECT;
    }
    graphics::check_hr(app.get(), dialog->SetOptions(options), "设置文件对话框选项");
    const auto caption = wide_text(title), suffix = wide_text(extension);
    graphics::check_hr(app.get(), dialog->SetTitle(caption.c_str()), "设置文件对话框标题");
    if (!suffix.empty())
    {
        graphics::check_hr(app.get(), dialog->SetDefaultExtension(suffix.c_str()), "设置默认扩展名");
    }
    std::vector<COMDLG_FILTERSPEC> specs;
    for (const auto& filter : filters)
    {
        specs.push_back({filter.name.c_str(), filter.pattern.c_str()});
    }
    if (!specs.empty())
    {
        graphics::check_hr(app.get(), dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data()), "设置文件过滤器");
    }
    owner_guard restore(window.hwnd);
    const auto status = dialog->Show(window.hwnd);
    if (status == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return {};
    }
    graphics::check_hr(app.get(), status, "显示文件对话框");
    std::vector<std::string> result;
    if (kind == file_dialog_kind::multiple)
    {
        IFileOpenDialog* open_raw = nullptr;
        graphics::check_hr(app.get(), dialog->QueryInterface(IID_IFileOpenDialog,
            reinterpret_cast<void**>(&open_raw)), "读取多选对话框");
        graphics::com_ptr<IFileOpenDialog> open(open_raw);
        IShellItemArray* array_raw = nullptr;
        graphics::check_hr(app.get(), open->GetResults(&array_raw), "读取多选文件");
        graphics::com_ptr<IShellItemArray> items(array_raw);
        DWORD count = 0;
        graphics::check_hr(app.get(), items->GetCount(&count), "读取文件数量");
        if (count > 4096)
        {
            fail("resource_limit", "一次最多选择 4096 个文件");
        }
        for (DWORD index = 0; index < count; ++index)
        {
            IShellItem* item_raw = nullptr;
            graphics::check_hr(app.get(), items->GetItemAt(index, &item_raw), "读取选择项");
            graphics::com_ptr<IShellItem> item(item_raw);
            result.push_back(item_path(*app, *item));
        }
    }
    else
    {
        IShellItem* item_raw = nullptr;
        graphics::check_hr(app.get(), dialog->GetResult(&item_raw), "读取选择文件");
        graphics::com_ptr<IShellItem> item(item_raw);
        result.push_back(item_path(*app, *item));
    }
    return result;
}

std::string message_box(graphics::window& owner, const std::string& title,
    const std::string& text, const std::string& buttons)
{
    require_system_idle(owner);
    UINT flags = MB_ICONINFORMATION;
    if (buttons == "ok")
    {
        flags |= MB_OK;
    }
    else if (buttons == "ok_cancel")
    {
        flags |= MB_OKCANCEL;
    }
    else if (buttons == "yes_no")
    {
        flags |= MB_YESNO;
    }
    else if (buttons == "yes_no_cancel")
    {
        flags |= MB_YESNOCANCEL;
    }
    else
    {
        fail("invalid_argument", "消息框按钮必须为 ok/ok_cancel/yes_no/yes_no_cancel");
    }
    const auto caption = wide_text(title), body = graphics::utf16(text);
    owner_guard restore(owner.hwnd);
    const auto result = MessageBoxW(owner.hwnd, body.c_str(), caption.c_str(), flags);
    if (!result)
    {
        platform_fail(owner.owner.lock().get(), "显示消息框", GetLastError());
    }
    return result == IDOK ? "ok" : result == IDYES ? "yes" : result == IDNO ? "no" : "cancel";
}

} // namespace tx_generated::gui
