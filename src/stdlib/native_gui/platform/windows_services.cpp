#include "stdlib/native_gui/platform/windows_window.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <shobjidl.h>

namespace tx_generated::native_gui
{
namespace
{
struct release_com
{
    template<class interface_type>
    void operator()(interface_type* value) const noexcept
    {
        if (value)
        {
            value->Release();
        }
    }
};
}
void platform_modal(window& dialog, window& owner, bool enabled)
{
    const auto dialog_hwnd = static_cast<tx::ui::windows_window&>(*dialog.host).hwnd;
    const auto owner_hwnd = static_cast<tx::ui::windows_window&>(*owner.host).hwnd;
    SetWindowLongPtrW(dialog_hwnd, GWLP_HWNDPARENT, enabled ? reinterpret_cast<LONG_PTR>(owner_hwnd) : 0);
    EnableWindow(owner_hwnd, !enabled);
}

tx::ui::point screen_origin(window& owner)
{
    POINT origin{};
    if (!ClientToScreen(static_cast<tx::ui::windows_window&>(*owner.host).hwnd, &origin))
    {
        fail("platform_error", "无法读取窗口屏幕坐标");
    }
    return {static_cast<double>(origin.x), static_cast<double>(origin.y)};
}

std::optional<std::string> file_dialog(window& owner, const std::string& kind,
    const std::string& title, const std::string& initial)
{
    if (kind != "open" && kind != "save" && kind != "folder")
    {
        fail("invalid_argument", "文件对话框类型必须为 open/save/folder");
    }
    if (!owner.modal_child.expired())
    {
        fail("modal_blocked", "owner 已被模态窗口阻塞");
    }
    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    struct com_guard
    {
        bool owned;
        ~com_guard()
        {
            if (owned)
            {
                CoUninitialize();
            }
        }
    } com{SUCCEEDED(initialized)};
    auto checked = [](HRESULT status)
    {
        if (FAILED(status))
        {
            fail("platform_error", "系统文件选择器失败，HRESULT=" + std::to_string(status));
        }
    };
    IFileDialog* raw = nullptr;
    checked(CoCreateInstance(kind == "save" ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
        nullptr, CLSCTX_INPROC_SERVER, IID_IFileDialog, reinterpret_cast<void**>(&raw)));
    std::unique_ptr<IFileDialog, release_com> dialog(raw);
    DWORD options = FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
    options |= kind == "save" ? FOS_OVERWRITEPROMPT : kind == "folder" ? FOS_PICKFOLDERS : FOS_FILEMUSTEXIST;
    checked(dialog->SetOptions(options));
    const auto caption = tx::ui::utf16_text(tx::ui::decode_utf8(title).scalars);
    checked(dialog->SetTitle(caption.c_str()));
    if (!initial.empty())
    {
        const auto name = tx::ui::utf16_text(tx::ui::decode_utf8(initial).scalars);
        checked(dialog->SetFileName(name.c_str()));
    }
    const auto hwnd = static_cast<tx::ui::windows_window&>(*owner.host).hwnd;
    struct modal_guard
    {
        window& owner;
        ~modal_guard()
        {
            owner.system_modal = false;
            owner.repaint = true;
        }
    } modal{owner};
    owner.system_modal = true;
    if (owner.native_gui_root)
    {
        reset_interaction(*owner.native_gui_root);
    }
    const auto status = dialog->Show(hwnd);
    if (status == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return {};
    }
    checked(status);
    IShellItem* item_raw = nullptr;
    checked(dialog->GetResult(&item_raw));
    std::unique_ptr<IShellItem, release_com> item(item_raw);
    PWSTR path = nullptr;
    checked(item->GetDisplayName(SIGDN_FILESYSPATH, &path));
    const auto text = tx::ui::utf32_text(path);
    CoTaskMemFree(path);
    return tx::ui::encode_utf8(text);
}
}
