#include "backend/cpp/error_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/text_reference.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

extern "C" int txrt_value_box_i64(std::int64_t value, void** result) noexcept;
extern "C" void txrt_value_release(void* value) noexcept;

namespace
{

using tx_generated::detail::runtime_context;

struct context_scope
{
    runtime_context context;
    runtime_context* previous = tx_generated::detail::thread_context;

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

struct error_on_delete
{
    ~error_on_delete()
    {
        tx_generated::detail::set_error(tx::error_kind::runtime,
            "cleanup_error", "析构时的新错误");
    }
};

void check_value_roots(runtime_context& context)
{
    void* boxed = nullptr;
    require(txrt_value_box_i64(42, &boxed) == 0, "整数装箱失败");
    auto* record = static_cast<tx_generated::detail::value_handle_record*>(
        static_cast<std::any*>(boxed));
    require(context.newest_handle ==
        static_cast<tx_generated::detail::handle_link*>(record),
        "普通值根链表指向错误");
    require(std::any_cast<std::int64_t>(*static_cast<std::any*>(boxed)) == 42,
        "普通值载荷错误");
    txrt_value_release(boxed);
    require(context.newest_handle == nullptr, "普通值根没有注销");

    auto* owned = tx_generated::detail::make_handle<std::any>(
        std::make_shared<error_on_delete>());
    tx_generated::detail::set_error(tx::error_kind::io,
        "original_error", "原始错误");
    tx_generated::detail::destroy_handle(owned);
    require(context.newest_handle == nullptr, "析构后的根没有注销");
    require(context.last_error_kind == tx::error_kind::io &&
        std::string(context.last_error_code) == "original_error" &&
        std::string(context.last_error) == "原始错误",
        "最后释放覆盖了原始错误");

    tx_generated::detail::make_handle<std::any>(
        std::make_shared<error_on_delete>());
    tx_generated::detail::cleanup_live_handles();
    require(context.newest_handle == nullptr, "退出清理后的根没有注销");
    require(context.last_error_kind == tx::error_kind::io &&
        std::string(context.last_error_code) == "original_error" &&
        std::string(context.last_error) == "原始错误",
        "退出清理覆盖了原始错误");
}

void check_cross_thread_text(runtime_context& context,
                             const std::string& contents, bool exit_cleanup)
{
    auto* text = tx_generated::detail::make_handle<std::string>(contents);
    tx_generated::text_reference retained(text);
    if (exit_cleanup)
    {
        tx_generated::detail::cleanup_live_handles();
    }
    else
    {
        tx_generated::detail::destroy_handle(text);
    }
    require(context.newest_handle == nullptr, "文本根没有注销");
    bool contents_valid = false;
    std::thread worker([reference = std::move(retained), contents,
                        &contents_valid]() mutable
    {
        contents_valid = reference.get() == contents;
        reference = tx_generated::text_reference();
    });
    worker.join();
    require(contents_valid, "跨线程文本内容错误");
}

} // namespace

int main()
{
    context_scope scope;
    check_value_roots(scope.context);
    check_cross_thread_text(scope.context, "短文本", false);
    check_cross_thread_text(scope.context, std::string(256, 'x'), true);
    return 0;
}
