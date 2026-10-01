#include "stdlib/profile_internal.hpp"
#include "stdlib/json.hpp"

#include <sstream>

namespace tx_generated::profiling
{
namespace
{

std::string quote(const std::string& value)
{
    // 标量 JSON 不建立 GC 对象；报告锁内不得再取得对象图登记锁。
    return json_stringify(std::any(value));
}

void source_fields(std::ostream& output, const detail::source_frame& source)
{
    output << "\"file\":" << quote(source.file)
           << ",\"function\":" << quote(source.function)
           << ",\"line\":" << source.line << ",\"column\":" << source.column;
}

} // namespace

std::string report_locked(profile_state& current)
{
    std::ostringstream output;
#ifdef _WIN32
    output << "{\"version\":1,\"platform\":\"windows-x64\","
           << "\"sampler\":\"thread_cpu_time_weighted_pc\",";
#else
    output << "{\"version\":1,\"platform\":\"linux-x64\","
           << "\"sampler\":\"thread_cpu_time_weighted_tx_frame\",";
#endif
    output << "\"interval_ms\":" << current.interval_ms
           << ",\"elapsed_us\":" << current.elapsed_us << ",\"sampler_us\":" << current.sampler_us
           << ",\"missed_samples\":" << current.missed_samples
           << ",\"allocation_scope\":\"cxx_new_since_start\",\"allocation_count\":" << current.allocation_count
           << ",\"allocated_bytes\":" << current.allocated_bytes
           << ",\"dropped_allocations\":" << current.dropped_allocations << ",\"cpu\":[";
    bool first = true;
    for (const auto& [key, sample] : current.samples)
    {
        (void)key;
        output << (first ? "{" : ",{");
        first = false;
        source_fields(output, sample.source);
        output << ",\"layer\":" << quote(sample.abi.empty()
            ? (sample.source.line ? "tx" : "native") : "stdlib_abi")
            << ",\"abi\":" << quote(sample.abi)
            << ",\"instruction_address\":" << quote(std::to_string(sample.instruction))
            << ",\"cpu_100ns\":" << sample.cpu_100ns << ",\"samples\":" << sample.count << '}';
    }
    output << "],\"heap\":[";
    std::map<std::string, std::pair<allocation, std::int64_t>> grouped;
    for (const auto& [pointer, item] : current.allocations)
    {
        (void)pointer;
        const auto key = std::string(item.source.file) + ":" +
            std::to_string(item.source.line) + ":" + item.source.function;
        auto& group = grouped[key];
        group.first.source = item.source;
        group.first.bytes += item.bytes;
        ++group.second;
    }
    first = true;
    for (const auto& [key, group] : grouped)
    {
        (void)key;
        output << (first ? "{" : ",{");
        first = false;
        source_fields(output, group.first.source);
        output << ",\"bytes\":" << group.first.bytes << ",\"allocations\":" << group.second << '}';
    }
    output << "],\"spans\":[";
    first = true;
    for (const auto& [id, item] : current.spans)
    {
        output << (first ? "{" : ",{") << "\"id\":" << id << ",\"name\":" << quote(item.name)
               << ",\"duration_us\":" << item.duration_us << '}';
        first = false;
    }
    return output.str() + "]}";
}

} // namespace tx_generated::profiling
