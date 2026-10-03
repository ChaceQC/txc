#include "stdlib/native_gui/state.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/theme.hpp"
#include "stdlib/native_gui/accessibility.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <limits>

namespace tx_generated::native_gui
{
namespace
{
std::filesystem::path font_path(const std::string& requested)
{
    std::string selected = requested;
    if (selected.empty())
    {
        if (const auto* environment = std::getenv("TX_GUI_FONT"))
        {
            selected = environment;
        }
    }
    if (!selected.empty())
    {
        return std::filesystem::path(std::u8string(selected.begin(), selected.end()));
    }
#ifdef _WIN32
    const auto* directory = std::getenv("SystemRoot");
    return std::filesystem::path(directory ? directory : "C:/Windows") / "Fonts/msyh.ttc";
#else
    for (const auto* candidate : {"/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"})
    {
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }
    fail("font_not_found", "未找到 TrueType 字体；请向 open_app 传入字体路径或设置 TX_GUI_FONT");
#endif
}

void poll_windows(app& state)
{
    for (const auto& window : state.windows)
    {
        if (window->closed)
        {
            continue;
        }
        window->accessibility->poll();
        for (unsigned count = 0; count < 64; ++count)
        {
            const auto pending = window->host->next_event(0);
            if (!pending)
            {
                break;
            }
            if (pending->kind == tx::ui::event_kind::close_requested)
            {
                if (window->modal_child.expired() && !window->system_modal)
                {
                    enqueue(*window, {"close_requested"});
                }
            }
            else if (pending->kind == tx::ui::event_kind::resized)
            {
                window->width = pending->width;
                window->height = pending->height;
                window->dpi = window->host->scale() * 96 * (window->native_gui_root ? window->native_gui_root->ui_scale : 1);
                window->minimized = !window->width || !window->height;
                window->repaint = true;
                enqueue(*window, {"resized"});
            }
            else if (pending->kind == tx::ui::event_kind::redraw)
            {
                window->repaint = true;
            }
            else if (pending->kind == tx::ui::event_kind::input_method_error)
            {
                event notification{"input_method_error"};
                notification.text = tx::ui::encode_utf8(pending->text);
                enqueue(*window, std::move(notification));
            }
            process_event(*window, *pending);
        }
        if (window->accessibility_platform)
        {
            window->accessibility_platform->poll(window->repaint);
        }
        if (window->repaint && window->visible && !window->minimized)
        {
            if (window->native_gui_root)
            {
                paint(*window->native_gui_root);
            }
            else
            {
                tx::ui::pixel_buffer image(window->width, window->height);
                image.clear({255, 255, 255, 255});
                window->host->present(image);
                window->repaint = false;
            }
        }
    }
}

bool animated(const node& state)
{
    if (!state.visible || !state.layout_visible || state.closed)
    {
        return false;
    }
    if (state.indeterminate)
    {
        return true;
    }
    return std::any_of(state.children.begin(), state.children.end(), [](const auto& child)
    {
        return animated(*child);
    });
}
}

app& require_app(graphics::resource* value, bool allow_closed)
{
    auto& state = *static_cast<app*>(value);
    require_thread(state.thread);
    if (!allow_closed && !state.open)
    {
        fail("closed_resource", "GUI 会话已经关闭");
    }
    return state;
}

window& require_window(graphics::resource* value, bool allow_closed)
{
    auto& state = *static_cast<window*>(value);
    const auto app = state.owner.lock();
    if (app)
    {
        require_thread(app->thread);
    }
    if (!allow_closed && (!app || !app->open || state.closed || !state.host->is_open()))
    {
        fail("closed_resource", "GUI 窗口已经关闭");
    }
    return state;
}

std::shared_ptr<app> open_app(const std::string& requested_font)
{
    auto& context = detail::current_runtime_context();
    if (!context.main_thread)
    {
        fail("wrong_thread", "GUI 会话必须在主线程初始化");
    }
    if (context.native_gui_session && static_cast<app*>(context.native_gui_session.get())->open)
    {
        fail("wrong_owner", "线程已经存在活动 GUI 会话");
    }
    auto state = std::shared_ptr<app>(new app, [](app* value)
    {
        close_app(*value);
        delete value;
    });
    state->font = std::make_shared<tx::ui::font_family>(tx::ui::system_font_family(font_path(requested_font)));
    poll_theme(*state);
    context.native_gui_cleanup = [](void* value) noexcept
    {
        close_app(*static_cast<app*>(value));
    };
    context.native_gui_session = state;
    return state;
}

std::shared_ptr<window> create_window(app& app, const std::string& title, double width, double height)
{
    checked_size(width);
    checked_size(height);
    if (width < 1 || height < 1 || app.windows.size() >= 64)
    {
        fail("resource_limit", "窗口尺寸必须为正，活动窗口不能超过 64 个");
    }
    auto result = std::make_shared<window>();
    result->host = tx::ui::create_platform_window(title, static_cast<unsigned>(width), static_cast<unsigned>(height));
    result->owner = app.shared_from_this();
    result->title = title;
    result->id = app.next_id++;
    result->dpi = result->host->scale() * 96;
    result->width = static_cast<unsigned>(width * result->host->scale());
    result->height = static_cast<unsigned>(height * result->host->scale());
    result->accessibility = std::make_shared<accessibility_endpoint>(result);
    result->accessibility_platform = connect_accessibility(*result);
    app.windows.push_back(result);
    return result;
}

void close_window(window& window) noexcept
{
    if (!window.closed)
    {
        if (const auto child = window.modal_child.lock())
        {
            close_window(*child);
        }
        close_menu(window);
        window.accessibility->disconnect();
        window.accessibility_platform.reset();
        close_root(window);
        restore_modal(window);
        window.host->close();
        window.closed = true;
    }
}

void close_app(app& app) noexcept
{
    if (!app.open)
    {
        return;
    }
    for (const auto& window : app.windows)
    {
        close_window(*window);
    }
    app.windows.clear();
    app.events.clear();
    app.font.reset();
    app.open = false;
}

void enqueue(window& window, event event)
{
    const auto app = window.owner.lock();
    if (!app || !app->open)
    {
        return;
    }
    event.window_id = window.id;
    if (!app->events.empty() && (event.kind == "text_changed" || event.kind == "value_changed" || event.kind == "resized"))
    {
        auto& previous = app->events.back();
        if (previous.kind == event.kind && previous.window_id == event.window_id && previous.source_id == event.source_id)
        {
            previous = std::move(event);
            return;
        }
    }
    if (app->events.size() >= 4096)
    {
        fail("resource_limit", "GUI 事件队列超过 4096 条");
    }
    app->events.push_back(std::move(event));
}

std::u32string clipboard_text(window& window)
{
    // 同一 UI 线程的另一个窗口可能拥有 X11 Selection，直接从拥有者读取，
    // 避免等待本线程自己处理 SelectionRequest 而互相阻塞。
    const auto app = window.owner.lock();
    for (const auto& candidate : app->windows)
    {
        if (!candidate->closed && candidate->host->owns_clipboard())
        {
            return candidate->host->clipboard_text();
        }
    }
    return window.host->clipboard_text();
}

std::optional<event> next_event(app& state, std::int64_t timeout_ms)
{
    if (timeout_ms < -1 || timeout_ms > std::numeric_limits<int>::max())
    {
        fail("invalid_argument", "事件等待时间超出范围");
    }
    const auto started = std::chrono::steady_clock::now();
    while (state.open)
    {
        poll_theme(state);
        bool animation = false;
        for (const auto& window : state.windows)
        {
            if (!window->closed && window->visible && !window->minimized && window->native_gui_root &&
                animated(*window->native_gui_root))
            {
                window->repaint = true;
                animation = true;
            }
        }
        poll_windows(state);
        if (!state.events.empty())
        {
            auto result = std::move(state.events.front());
            state.events.pop_front();
            return result;
        }
        std::erase_if(state.windows, [](const auto& window)
        {
            return window->closed;
        });
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        if (state.windows.empty() || (timeout_ms >= 0 && elapsed >= timeout_ms))
        {
            return {};
        }
        std::vector<tx::ui::platform_window*> windows;
        for (const auto& window : state.windows)
        {
            windows.push_back(window->host.get());
        }
        int wait = timeout_ms < 0 ? 20 : std::min(20, static_cast<int>(timeout_ms - elapsed));
        if (animation)
        {
            wait = wait < 0 ? 25 : std::min(wait, 25);
        }
        tx::ui::wait_platform_events(windows, wait);
    }
    return {};
}
}
