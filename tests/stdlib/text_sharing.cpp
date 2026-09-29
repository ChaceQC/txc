#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/text_reference.hpp"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{

struct context_scope
{
    tx_generated::detail::runtime_context context;
    tx_generated::detail::runtime_context* previous =
        tx_generated::detail::thread_context;

    context_scope()
    {
        tx_generated::detail::thread_context = &context;
    }

    ~context_scope()
    {
        tx_generated::detail::thread_context = previous;
    }
};

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

struct read_on_delete
{
    tx_generated::text_reference value;
    bool* observed;

    read_on_delete(tx_generated::text_reference text, bool* result)
        : value(std::move(text)), observed(result)
    {
    }

    ~read_on_delete()
    {
        *observed = value.get() == "析构仍可读取";
    }
};

void check_clone_and_container(const std::string& expected)
{
    using tx_generated::detail::text_value;
    auto* original = tx_generated::detail::make_handle<std::string>(expected);
    std::vector<tx_generated::text_reference> container;
    container.emplace_back(original);
    void* cloned = nullptr;
    require(txrt_str_clone(original, &cloned) == 0, "文本克隆失败");
    require(original != cloned, "克隆没有创建独立根");
    require(&text_value(original) == &text_value(cloned), "克隆复制了文本内容");

    txrt_str_release(original);
    txrt_str_release(cloned);
    require(container.front().get() == expected, "容器引用在根释放后失效");

    // 容器槽位中的借用指针也能在目标线程创建本地根。
    std::thread worker([reference = container.front(), expected]()
    {
        context_scope scope;
        void* local_root = nullptr;
        require(txrt_str_clone(reference.handle(), &local_root) == 0,
            "跨线程克隆失败");
        require(scope.context.newest_handle != nullptr, "目标线程未登记根");
        require(text_value(local_root) == expected, "跨线程文本内容错误");
        txrt_str_release(local_root);
        require(scope.context.newest_handle == nullptr, "目标线程根未注销");
    });
    worker.join();

    void* from_container = nullptr;
    require(txrt_str_clone(container.front().handle(), &from_container) == 0,
        "容器借用克隆失败");
    require(&text_value(from_container) == &container.front().get(),
        "容器克隆没有共享内容");
    txrt_str_release(from_container);
}

void check_builder()
{
    auto* builder = tx_generated::detail::make_text_builder();
    tx_generated::detail::text_builder(builder).append("foo");
    tx_generated::detail::text_builder(builder).append("bar");
    tx_generated::detail::publish_text_builder(builder);
    bool rejected = false;
    try
    {
        tx_generated::detail::text_builder(builder).append("changed");
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    require(rejected, "已发布文本仍可追加");
    void* cloned = nullptr;
    require(txrt_str_clone(builder, &cloned) == 0, "发布后的文本克隆失败");
    require(tx_generated::detail::text_value(cloned) == "foobar",
        "发布后的文本内容错误");
    require(&tx_generated::detail::text_value(builder) ==
        &tx_generated::detail::text_value(cloned), "发布后的文本未共享");
    txrt_str_release(builder);
    txrt_str_release(cloned);
}

void check_deinit_after_root_release()
{
    auto* root = tx_generated::detail::make_handle<std::string>("析构仍可读取");
    bool observed = false;
    auto owner = std::make_shared<read_on_delete>(
        tx_generated::text_reference(root), &observed);
    tx_generated::detail::make_handle<std::any>(owner);
    owner.reset();
    txrt_str_release(root);
    tx_generated::detail::cleanup_live_handles();
    require(observed, "根注销后析构无法读取文本");
}

} // namespace

int main()
{
    context_scope scope;
    check_clone_and_container("短文本");
    check_clone_and_container(std::string(4096, 'x'));
    check_builder();
    check_deinit_after_root_release();
    require(scope.context.newest_handle == nullptr, "主线程遗留文本根");
    return 0;
}
