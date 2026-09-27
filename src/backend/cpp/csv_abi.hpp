#pragma once

extern "C"
{

int txrt_csv_default_dialect(const char* type, void** result) noexcept;
int txrt_csv_reader_binary(const void* source, const void* config, void** result) noexcept;
int txrt_csv_reader_text(const void* source, const void* config, void** result) noexcept;
int txrt_csv_next_row(const void* source, const char* type, void** result) noexcept;
int txrt_csv_header(const void* source, void** result) noexcept;
int txrt_csv_writer_binary(const void* target, const void* config, void** result) noexcept;
int txrt_csv_writer_text(const void* target, const void* config, void** result) noexcept;
int txrt_csv_write_row(const void* target, const void* row) noexcept;
int txrt_csv_finish(const void* target) noexcept;
int txrt_csv_close_reader(const void* target) noexcept;
int txrt_csv_close_writer(const void* target) noexcept;
int txrt_csv_parse(const void* source, const void* config, void** result) noexcept;
int txrt_csv_stringify(const void* data, const void* config, void** result) noexcept;

}
