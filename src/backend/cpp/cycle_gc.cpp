#include "backend/cpp/cycle_gc.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace
{

constexpr std::size_t collection_interval = 64;

struct registered_node
{
    std::weak_ptr<void> object;
    gc_trace trace;
    gc_clear clear;
    gc_finalize finalize;
};

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
    std::unordered_map<const void*, std::size_t> positions;
    std::vector<std::vector<std::size_t>> edges;
    std::vector<std::int64_t> external_refs;
    std::vector<bool> reachable;
};

thread_local std::vector<registered_node> registered_nodes;
thread_local std::size_t allocations_since_collection = 0;
thread_local bool collecting = false;

const void* object_identity(const std::any& value)
{
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
    graph.edges[source].push_back(found->second);
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
        for (const auto target : graph.edges[source])
        {
            if (!graph.reachable[target])
            {
                graph.reachable[target] = true;
                pending.push_back(target);
            }
        }
    }
}

object_graph inspect_graph()
{
    object_graph graph;
    graph.nodes.reserve(registered_nodes.size());
    for (const auto& entry : registered_nodes)
    {
        if (auto object = entry.object.lock())
        {
            graph.nodes.push_back({std::move(object), entry.trace,
                                   entry.clear, entry.finalize});
        }
    }
    graph.edges.resize(graph.nodes.size());
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
        edge_context context{graph, index};
        graph.nodes[index].trace(graph.nodes[index].object.get(),
                                 append_edge, &context);
    }
    mark_reachable(graph);
    return graph;
}

struct collection_guard
{
    ~collection_guard()
    {
        collecting = false;
    }
};

} // namespace

void register_gc_node(const std::shared_ptr<void>& object, gc_trace trace,
                      gc_clear clear, gc_finalize finalize)
{
    registered_nodes.push_back({object, trace, clear, finalize});
    ++allocations_since_collection;
}

void note_gc_allocation() noexcept
{
    ++allocations_since_collection;
}

void collect_cycles()
{
    if (collecting)
    {
        return;
    }
    collecting = true;
    collection_guard guard;
    bool completed = false;
    for (std::size_t round = 0; round < 16; ++round)
    {
        auto graph = inspect_graph();
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
    std::erase_if(registered_nodes, [](const registered_node& entry)
    {
        return entry.object.expired();
    });
    allocations_since_collection = completed ? 0 :
        std::max(collection_interval, registered_nodes.size() / 2);
}

void gc_safepoint()
{
    if (collecting || registered_nodes.empty())
    {
        return;
    }
    const auto threshold = std::max(collection_interval,
                                    registered_nodes.size() / 2);
    if (allocations_since_collection >= threshold)
    {
        collect_cycles();
    }
}

} // namespace tx_generated

extern "C" int txrt_gc_safepoint() noexcept
{
    return tx_generated::detail::invoke_checked([]
    {
        tx_generated::gc_safepoint();
    });
}
