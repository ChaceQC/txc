#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/identity_table.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace
{

constexpr std::size_t collection_interval = 64;
std::atomic<std::uint64_t> next_gc_owner_id = 1;
std::atomic<std::size_t> registered_nodes_hint = 0;

struct live_node
{
    std::shared_ptr<void> object;
    gc_trace trace;
    gc_clear clear;
    gc_finalize finalize;
};

struct object_graph
{
    std::vector<live_node> nodes;
    identity_table<std::size_t, 64> positions;
    std::vector<std::size_t> edges;
    std::vector<std::size_t> edge_offsets;
    std::vector<std::int64_t> external_refs;
    std::vector<bool> reachable;
};

struct gc_registry
{
    std::mutex mutex;
    std::vector<detail::registered_gc_node> nodes;
};

gc_registry& registered_graph()
{
    static auto* registry = new gc_registry();
    return *registry;
}

std::shared_mutex& execution_gate()
{
    static auto* gate = new std::shared_mutex();
    return *gate;
}

std::size_t registered_node_count()
{
    auto& registry = registered_graph();
    std::lock_guard lock(registry.mutex);
    return registry.nodes.size();
}

const void* object_identity(const std::any& value)
{
    if (!value.has_value() || value.type() == typeid(std::int64_t) ||
        value.type() == typeid(double) || value.type() == typeid(bool) ||
        value.type() == typeid(std::string))
    {
        return nullptr;
    }
    if (value.type() == typeid(tx_array))
    {
        return std::any_cast<const tx_array&>(value).identity();
    }
    if (value.type() == typeid(tx_dict))
    {
        return std::any_cast<const tx_dict&>(value).identity();
    }
    if (value.type() == typeid(dynamic_struct))
    {
        return std::any_cast<const dynamic_struct&>(value).identity();
    }
    if (value.type() == typeid(class_handle))
    {
        return std::any_cast<const class_handle&>(value).operator->();
    }
    if (value.type() == typeid(object_vector))
    {
        return std::any_cast<const object_vector&>(value).identity();
    }
    if (value.type() == typeid(container_handle))
    {
        return std::any_cast<const container_handle&>(value).get();
    }
    if (value.type() == typeid(tx_iterator))
    {
        return std::any_cast<const tx_iterator&>(value).identity();
    }
    if (value.type() == typeid(closure_handle))
    {
        return std::any_cast<const closure_handle&>(value).identity();
    }
    return nullptr;
}

struct edge_context
{
    object_graph& graph;
    std::size_t source;
};

void append_edge(const std::any& value, void* context)
{
    auto& [graph, source] = *static_cast<edge_context*>(context);
    const auto found = graph.positions.find(object_identity(value));
    if (found == graph.positions.end())
    {
        return;
    }
    (void)source;
    graph.edges.push_back(found->second);
    --graph.external_refs[found->second];
}

void mark_reachable(object_graph& graph)
{
    std::vector<std::size_t> pending;
    for (std::size_t index = 0; index < graph.nodes.size(); ++index)
    {
        if (graph.external_refs[index] > 0)
        {
            graph.reachable[index] = true;
            pending.push_back(index);
        }
    }
    while (!pending.empty())
    {
        const auto source = pending.back();
        pending.pop_back();
        for (auto edge = graph.edge_offsets[source]; edge < graph.edge_offsets[source + 1]; ++edge)
        {
            const auto target = graph.edges[edge];
            if (!graph.reachable[target])
            {
                graph.reachable[target] = true;
                pending.push_back(target);
            }
        }
    }
}

std::uint64_t ensure_gc_owner_id(detail::runtime_context& context)
{
    if (context.gc_owner_id == 0)
    {
        context.gc_owner_id = next_gc_owner_id.fetch_add(
            1, std::memory_order_relaxed);
    }
    return context.gc_owner_id;
}

object_graph inspect_graph(std::uint64_t owner)
{
    auto& registry = registered_graph();
    object_graph graph;
    {
        std::lock_guard lock(registry.mutex);
        if (owner == 0)
        {
            graph.nodes.reserve(registry.nodes.size());
        }
        for (const auto& entry : registry.nodes)
        {
            // owner 过滤先于 weak_ptr::lock，不复制无关线程的登记项。
            if (owner != 0 && entry.owner != owner)
            {
                continue;
            }
            if (auto object = entry.object.lock())
            {
                graph.nodes.push_back({std::move(object), entry.trace,
                                       entry.clear, entry.finalize});
            }
        }
    }
    graph.edge_offsets.reserve(graph.nodes.size() + 1);
    graph.edges.reserve(graph.nodes.size());
    graph.positions.reserve(graph.nodes.size());
    graph.external_refs.reserve(graph.nodes.size());
    graph.reachable.resize(graph.nodes.size(), false);
    for (std::size_t index = 0; index < graph.nodes.size(); ++index)
    {
        graph.positions.emplace(graph.nodes[index].object.get(), index);
        // 快照本身恰好增加一个强引用，不属于程序根。
        graph.external_refs.push_back(
            static_cast<std::int64_t>(graph.nodes[index].object.use_count()) - 1);
    }
    for (std::size_t index = 0; index < graph.nodes.size(); ++index)
    {
        graph.edge_offsets.push_back(graph.edges.size());
        edge_context context{graph, index};
        graph.nodes[index].trace(graph.nodes[index].object.get(),
                                 append_edge, &context);
    }
    graph.edge_offsets.push_back(graph.edges.size());
    mark_reachable(graph);
    return graph;
}

struct collection_guard
{
    detail::runtime_context& context;

    ~collection_guard()
    {
        context.collecting = false;
    }
};

} // namespace

concurrent_execution_scope::concurrent_execution_scope()
{
    context_ = &detail::current_runtime_context();
    if (context_->concurrent_depth == 0)
    {
        // 锁跟随栈上作用域释放，避免 MinGW 在线程退出时析构非平凡 TLS 对象。
        execution_lock_ = std::shared_lock<std::shared_mutex>(execution_gate());
    }
    ++context_->concurrent_depth;
}

concurrent_execution_scope::~concurrent_execution_scope()
{
    --context_->concurrent_depth;
}

void register_gc_node(const std::shared_ptr<void>& object, gc_trace trace,
                      gc_clear clear, gc_finalize finalize)
{
    auto& registry = registered_graph();
    auto& context = detail::current_runtime_context();
    const auto owner = ensure_gc_owner_id(context);
    {
        std::lock_guard lock(registry.mutex);
        registry.nodes.push_back({object, trace, clear, finalize, owner});
        // 登记完成后发布数量提示；真正扫描仍在锁内取快照。
        registered_nodes_hint.store(registry.nodes.size(),
                                    std::memory_order_release);
    }
    ++context.allocations_since_collection;
}

void note_gc_allocation() noexcept
{
    ++detail::current_runtime_context().allocations_since_collection;
}

void collect_cycles_for_context(detail::runtime_context& context)
{
    if (context.collecting)
    {
        return;
    }
    std::optional<std::unique_lock<std::shared_mutex>> gate;
    const auto owner_filter = context.concurrent_depth != 0
        ? ensure_gc_owner_id(context) : 0;
    if (owner_filter == 0)
    {
        gate.emplace(execution_gate(), std::try_to_lock);
        if (!gate->owns_lock())
        {
            // 活跃工作线程可能正在改写其唯一移动的对象图；延后全图扫描。
            return;
        }
    }
    context.collecting = true;
    collection_guard guard{context};
    bool completed = false;
    for (std::size_t round = 0; round < 16; ++round)
    {
        auto graph = inspect_graph(owner_filter);
        bool finalized = false;
        for (std::size_t index = 0; index < graph.nodes.size(); ++index)
        {
            if (!graph.reachable[index] && graph.nodes[index].finalize)
            {
                finalized |= graph.nodes[index].finalize(
                    graph.nodes[index].object);
            }
        }
        if (finalized)
        {
            // deinit 可以改变引用图或复活对象，必须重新判定可达性。
            continue;
        }
        for (std::size_t index = 0; index < graph.nodes.size(); ++index)
        {
            if (!graph.reachable[index])
            {
                graph.nodes[index].clear(graph.nodes[index].object.get());
            }
        }
        completed = true;
        break;
    }
    auto& registry = registered_graph();
    {
        std::lock_guard lock(registry.mutex);
        std::erase_if(registry.nodes,
            [](const detail::registered_gc_node& entry)
            {
                return entry.object.expired();
            });
        registered_nodes_hint.store(registry.nodes.size(),
                                    std::memory_order_release);
    }
    context.allocations_since_collection = completed ? 0 :
        std::max(collection_interval, registered_node_count() / 2);
}

void collect_cycles()
{
    collect_cycles_for_context(detail::current_runtime_context());
}

void gc_safepoint(detail::runtime_context& context)
{
    if (context.collecting || context.concurrent_depth != 0 ||
        context.allocations_since_collection < collection_interval)
    {
        return;
    }
    // 数量只是调度提示：锁内再读真实数量，进入回收时仍取完整快照。
    const auto hinted_nodes = registered_nodes_hint.load(
        std::memory_order_acquire);
    if (hinted_nodes == 0 || context.allocations_since_collection <
        std::max(collection_interval, hinted_nodes / 2))
    {
        return;
    }
    const auto registered_nodes = registered_node_count();
    if (registered_nodes == 0)
    {
        return;
    }
    const auto threshold = std::max(collection_interval, registered_nodes / 2);
    if (context.allocations_since_collection >= threshold)
    {
        collect_cycles_for_context(context);
    }
}

void gc_safepoint()
{
    gc_safepoint(detail::current_runtime_context());
}

} // namespace tx_generated

extern "C" int txrt_gc_safepoint() noexcept
{
    return tx_generated::detail::invoke_checked([]
    {
        tx_generated::gc_safepoint();
    });
}

extern "C" int txrt_gc_safepoint_context(void* raw_context) noexcept
{
    auto& context = *static_cast<tx_generated::detail::runtime_context*>(
        raw_context);
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::gc_safepoint(context);
    }, tx::error_kind::runtime, &context);
}
