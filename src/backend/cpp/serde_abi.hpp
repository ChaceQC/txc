#pragma once

extern "C"
{

int txrt_serde_serialize_json(const char* schema, const void* value,
                              void** result) noexcept;
int txrt_serde_deserialize_json(const char* schema, const void* text,
                                void** result) noexcept;
int txrt_serde_serialize_cbor(const char* schema, const void* value,
                              void** result) noexcept;
int txrt_serde_deserialize_cbor(const char* schema, const void* bytes,
                                void** result) noexcept;

}
