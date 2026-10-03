#pragma once

#include "stdlib/native_gui/state.hpp"

#include <functional>
#include <future>
#include <map>
#include <mutex>

namespace tx_generated::native_gui
{
struct accessible_node
{
    std::string id, parent, name, help, role;
    std::vector<std::string> children;
    rectangle bounds;
    std::u32string text;
    std::size_t selection_start = 0, selection_end = 0, caret = 0;
    bool enabled = false, visible = false, focused = false, focusable = false;
    bool invoke = false, editable = false, password = false, range = false, selectable = false;
    bool selected = false, checked = false, checkable = false, multiple = false, text_capable = false;
    double value = 0, minimum = 0, maximum = 0, step = 0;
    std::int64_t node_id = 0, item_id = 0;
    int item_kind = 0;
};
using accessible_tree = std::map<std::string, accessible_node>;

struct accessible_action
{
    accessible_action(std::string id, std::string name) : target(std::move(id)), operation(std::move(name))
    {
    }
    std::string target, operation;
    std::u32string text;
    double value = 0;
    std::int64_t start = 0, end = 0;
};

class accessibility_endpoint : public std::enable_shared_from_this<accessibility_endpoint>
{
public:
    explicit accessibility_endpoint(std::weak_ptr<window> owner);
    accessible_tree tree();
    bool action(const accessible_action& request);
    std::vector<rectangle> text_bounds(const std::string& id, std::size_t begin, std::size_t end);
    std::size_t text_at_point(const std::string& id, tx::ui::point point);
    std::vector<std::size_t> text_units(const std::string& id, unsigned unit);
    std::vector<std::pair<std::size_t, std::size_t>> visible_text_ranges(const std::string& id);
    void poll();
    void disconnect();
    std::string root_id;
private:
    std::weak_ptr<window> owner_;
    std::thread::id thread_;
    std::mutex mutex_;
    std::deque<std::function<void()>> pending_;
    bool connected_ = true;
    template<class operation>
    auto call(operation run) -> decltype(run(std::declval<window&>()))
    {
        using result_type = decltype(run(std::declval<window&>()));
        auto operation_task = std::make_shared<std::packaged_task<result_type()>>([owner = owner_, run]
        {
            const auto value = owner.lock();
            if (!value || value->closed)
            {
                throw std::runtime_error("辅助对象已经关闭");
            }
            return run(*value);
        });
        auto future = operation_task->get_future();
        if (std::this_thread::get_id() == thread_)
        {
            (*operation_task)();
        }
        else
        {
            std::lock_guard lock(mutex_);
            if (!connected_ || pending_.size() >= 256)
            {
                throw std::runtime_error("辅助对象不可用或请求过多");
            }
            pending_.push_back([operation_task]
            {
                (*operation_task)();
            });
        }
        if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
        {
            throw std::runtime_error("GUI 线程没有及时处理辅助技术请求");
        }
        return future.get();
    }
};

class accessibility_bridge
{
public:
    virtual ~accessibility_bridge() = default;
    virtual void poll(bool changed) = 0;
};
std::shared_ptr<accessibility_bridge> connect_accessibility(window& owner);
accessible_tree build_accessible_tree(window& owner);
void accessible_menu_tree(window& owner, accessible_tree& tree, tx::ui::point origin);
bool perform_accessible_action(window& owner, const accessible_action& action);
std::shared_ptr<node> find_node(window& owner, std::int64_t id);
void set_accessibility(node& state, const std::string& name, const std::string& help);
}
