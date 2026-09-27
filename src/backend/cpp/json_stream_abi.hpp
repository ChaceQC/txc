#pragma once

extern "C"
{

int txrt_json_default_limits(const char* type, void** result) noexcept;
int txrt_json_read_binary(const void* source, const void* limits, void** result) noexcept;
int txrt_json_read_text(const void* source, const void* limits, void** result) noexcept;
int txrt_json_reader_binary(const void* source, const void* limits, void** result) noexcept;
int txrt_json_reader_text(const void* source, const void* limits, void** result) noexcept;
int txrt_json_next(const void* source, const char* type, void** result) noexcept;
int txrt_json_write_binary(const void* target, const void* value, const void* limits) noexcept;
int txrt_json_write_text(const void* target, const void* value, const void* limits) noexcept;
int txrt_json_writer_binary(const void* target, const void* limits, void** result) noexcept;
int txrt_json_writer_text(const void* target, const void* limits, void** result) noexcept;
int txrt_json_write_value(const void* target, const void* value) noexcept;
int txrt_json_finish(const void* target) noexcept;
int txrt_json_close_reader(const void* target) noexcept;
int txrt_json_close_writer(const void* target) noexcept;
int txrt_json_validate(const void* value, const void* schema) noexcept;

}
