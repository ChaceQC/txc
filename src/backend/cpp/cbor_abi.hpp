#pragma once

#include <cstdint>

extern "C"
{

int txrt_cbor_default_limits(const char* type, void** result) noexcept;
int txrt_cbor_encode(const void* value, const void* limits, void** result) noexcept;
int txrt_cbor_decode(const void* data, const void* limits, void** result) noexcept;
int txrt_cbor_read(const void* source, const void* limits, void** result) noexcept;
int txrt_cbor_write(const void* target, const void* value, const void* limits) noexcept;
int txrt_cbor_reader(const void* source, const void* limits, void** result) noexcept;
int txrt_cbor_next(const void* reader, const char* type, void** result) noexcept;
int txrt_cbor_writer(const void* target, std::int64_t count, const void* limits,
                    void** result) noexcept;
int txrt_cbor_write_value(const void* writer, const void* value) noexcept;
int txrt_cbor_finish(const void* writer) noexcept;
int txrt_cbor_close_reader(const void* reader) noexcept;
int txrt_cbor_close_writer(const void* writer) noexcept;

}
