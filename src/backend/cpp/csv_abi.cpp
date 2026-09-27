#include "backend/cpp/csv_abi.hpp"
#include "backend/cpp/format_abi.hpp"
#include "stdlib/csv.hpp"
#include "stdlib/vector.hpp"

using namespace tx_generated;
using namespace tx_generated::format_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

namespace
{

csv_dialect read_dialect(const void* config)
{
    return {member<std::string>(config, 0), member<std::string>(config, 1),
        member<std::string>(config, 2), member<bool>(config, 3),
        member<bool>(config, 4), member<bool>(config, 5), member<bool>(config, 6),
        member<std::int64_t>(config, 7), member<std::int64_t>(config, 8),
        member<std::int64_t>(config, 9), member<std::int64_t>(config, 10),
        member<std::int64_t>(config, 11)};
}

string_vector row_value(csv_row row)
{
    string_vector result;
    result.data().values.reserve(row.size());
    for (auto& item : row)
    {
        auto* handle = make_handle<std::string>(std::move(item));
        result.data().values.emplace_back(handle);
        detail::destroy_handle(handle);
    }
    result.data().refresh();
    return result;
}

csv_row row_argument(const string_vector& row)
{
    csv_row result;
    result.reserve(row.data().values.size());
    for (const auto& item : row.data().values)
    {
        result.push_back(item.get());
    }
    return result;
}

} // namespace

extern "C" int txrt_csv_default_dialect(const char* type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const csv_dialect config;
        *result = structure(type, "dialect", {
            {"delimiter", config.delimiter}, {"quote", config.quote}, {"newline", config.newline},
            {"has_header", config.has_header}, {"strict_width", config.strict_width},
            {"allow_bom", config.allow_bom}, {"write_bom", config.write_bom},
            {"max_field_bytes", config.max_field_bytes}, {"max_row_bytes", config.max_row_bytes},
            {"max_columns", config.max_columns}, {"max_bytes", config.max_bytes},
            {"max_rows", config.max_rows}});
    });
}

extern "C" int txrt_csv_next_row(const void* source, const char* type, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto row = csv_next_row(argument<csv_reader>(source));
        std::optional<std::any> item;
        if (row)
        {
            item = row_value(std::move(*row));
        }
        *result = option(type, std::move(item));
    });
}

extern "C" int txrt_csv_header(const void* source, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(row_value(csv_header(argument<csv_reader>(source))));
    });
}

extern "C" int txrt_csv_write_row(const void* target, const void* row) noexcept
{
    return invoke_checked([&]
    {
        csv_write_row(argument<csv_writer>(target), row_argument(argument<string_vector>(row)));
    });
}

extern "C" int txrt_csv_finish(const void* target) noexcept
{
    return invoke_checked([&]
    {
        csv_finish(argument<csv_writer>(target));
    });
}

extern "C" int txrt_csv_close_reader(const void* target) noexcept
{
    return invoke_checked([&]
    {
        csv_close(argument<csv_reader>(target));
    });
}

extern "C" int txrt_csv_close_writer(const void* target) noexcept
{
    return invoke_checked([&]
    {
        csv_close(argument<csv_writer>(target));
    });
}

extern "C" int txrt_csv_parse(const void* source, const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto rows = csv_parse(text(source), read_dialect(config));
        object_vector output("vector<str>");
        output.data().values.reserve(rows.size());
        for (auto& row : rows)
        {
            output.data().values.push_back(row_value(std::move(row)));
        }
        output.data().refresh();
        *result = make_handle<std::any>(std::move(output));
    });
}

extern "C" int txrt_csv_stringify(const void* data, const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        std::vector<csv_row> rows;
        const auto& source = argument<object_vector>(data).data().values;
        rows.reserve(source.size());
        for (const auto& row : source)
        {
            rows.push_back(row_argument(std::any_cast<const string_vector&>(row)));
        }
        *result = make_handle<std::string>(csv_stringify(rows, read_dialect(config)));
    });
}

extern "C" int txrt_csv_reader_binary(const void* source, const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(source);
        *result = make_handle<std::any>(csv_new_reader(
            format_read_source(stream), read_dialect(config), format_stream_guard(stream)));
    });
}

extern "C" int txrt_csv_writer_binary(const void* target, const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(target);
        *result = make_handle<std::any>(csv_new_writer(format_write_sink(stream), [stream]
        {
            stream_flush(stream);
        }, read_dialect(config)));
    });
}

extern "C" int txrt_csv_reader_text(const void* source, const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(source);
        *result = make_handle<std::any>(csv_new_reader(
            format_read_source(stream), read_dialect(config), format_stream_guard(stream)));
    });
}

extern "C" int txrt_csv_writer_text(const void* target, const void* config, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(target);
        *result = make_handle<std::any>(csv_new_writer(format_write_sink(stream), [stream]
        {
            stream_flush(stream);
        }, read_dialect(config)));
    });
}
