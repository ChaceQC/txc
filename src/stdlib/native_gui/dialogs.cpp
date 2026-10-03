#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/commands.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
void show_modal(window& dialog, window& owner)
{
    if (&dialog == &owner || dialog.owner.lock() != owner.owner.lock())
    {
        fail("wrong_owner", "模态窗口必须与不同的 owner 属于同一应用");
    }
    if (!dialog.modal_owner.expired() || !owner.modal_child.expired())
    {
        fail("modal_blocked", "窗口已经处于模态关系中");
    }
    for (auto current = owner.shared_from_this(); current; current = current->modal_owner.lock())
    {
        if (current.get() == &dialog)
        {
            fail("wrong_owner", "模态窗口所有权不能循环");
        }
    }
    if (owner.native_gui_root)
    {
        owner.modal_focus = owner.native_gui_root->focused;
        reset_interaction(*owner.native_gui_root);
        owner.host->enable_ime(false);
    }
    platform_modal(dialog, owner, true);
    dialog.modal_owner = owner.shared_from_this();
    owner.modal_child = dialog.shared_from_this();
    try
    {
        dialog.host->show();
        dialog.host->focus();
        dialog.visible = true;
        dialog.repaint = true;
    }
    catch (...)
    {
        restore_modal(dialog);
        throw;
    }
}

void restore_modal(window& dialog) noexcept
{
    const auto owner = dialog.modal_owner.lock();
    dialog.modal_owner.reset();
    if (!owner)
    {
        return;
    }
    owner->modal_child.reset();
    if (owner->closed)
    {
        return;
    }
    try
    {
        platform_modal(dialog, *owner, false);
        owner->host->focus();
        if (owner->native_gui_root)
        {
            const auto focused = owner->modal_focus.lock();
            // 强制刷新 IME 状态，即使逻辑焦点仍是同一节点。
            owner->native_gui_root->focused.reset();
            focus_node(*owner->native_gui_root, focused && available(*focused) ? focused : nullptr);
        }
        owner->repaint = true;
    }
    catch (...)
    {
    }
    owner->modal_focus.reset();
}

void end_dialog(window& dialog, std::int64_t result)
{
    dialog.dialog_result = result;
    close_window(dialog);
}

std::string message_box(window& owner, const std::string& title, const std::string& text, const std::string& buttons)
{
    std::vector<std::pair<std::string, std::string>> choices;
    if (buttons == "ok" || buttons == "ok_cancel")
    {
        choices.push_back({"确定", "ok"});
    }
    else if (buttons == "yes_no" || buttons == "yes_no_cancel")
    {
        choices = {{"是", "yes"}, {"否", "no"}};
    }
    else
    {
        fail("invalid_argument", "消息按钮必须为 ok/ok_cancel/yes_no/yes_no_cancel");
    }
    if (buttons.ends_with("cancel"))
    {
        choices.push_back({"取消", "cancel"});
    }
    auto app = owner.owner.lock();
    const auto dialog = create_window(*app, title, 520, 260);
    struct cleanup
    {
        std::shared_ptr<window> dialog;
        ~cleanup()
        {
            close_window(*dialog);
        }
    } guard{dialog};
    auto panel = root(*dialog);
    panel->semantic_role = "dialog";
    auto body = create(*panel, tx::graphics_kind::native_label, text);
    body->height = {gui::length_mode::stretch, 1};
    auto actions = create(*panel, tx::graphics_kind::native_panel, "");
    actions->layout = layout_mode::row;
    std::vector<std::int64_t> ids;
    for (const auto& choice : choices)
    {
        auto button = create(*actions, tx::graphics_kind::native_button, choice.first);
        button->width = {gui::length_mode::stretch, 1};
        ids.push_back(button->id);
        if (ids.size() == 1)
        {
            set_action_button(*button, false, true);
        }
        if (choice.second == "cancel")
        {
            set_action_button(*button, true, true);
        }
    }
    show_modal(*dialog, owner);
    std::deque<event> deferred;
    struct restore_events
    {
        native_gui::app& application;
        std::deque<event>& events;
        ~restore_events()
        {
            application.events.insert(application.events.begin(),
                std::make_move_iterator(events.begin()), std::make_move_iterator(events.end()));
        }
    } restore{*app, deferred};
    while (!dialog->closed && app->open)
    {
        const auto input = next_event(*app, 100);
        if (!input)
        {
            continue;
        }
        if (input->window_id != dialog->id)
        {
            deferred.push_back(*input);
        }
        else if (input->kind == "close_requested")
        {
            return "cancel";
        }
        else if (input->kind == "activated")
        {
            const auto found = std::find(ids.begin(), ids.end(), input->source_id);
            if (found != ids.end())
            {
                return choices[found - ids.begin()].second;
            }
        }
    }
    return "cancel";
}
}
