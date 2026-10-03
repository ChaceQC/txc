#pragma once

#include <cstdint>
#include <memory>
#include <string>

// sd-bus 仅作为操作系统 D-Bus 传输；控件语义和 AT-SPI 协议由本项目实现。
struct sd_bus;
struct sd_bus_message;
struct sd_bus_slot;
struct sd_bus_error
{
    const char* name;
    const char* message;
    int need_free;
};

namespace tx::ui
{
using bus_handler = int (*)(sd_bus_message*, void*, sd_bus_error*);
struct bus_api
{
    bus_api();
    ~bus_api();
    void* library = nullptr;
    int (*open_user)(sd_bus**) = nullptr;
    int (*create)(sd_bus**) = nullptr;
    int (*set_address)(sd_bus*, const char*) = nullptr;
    int (*set_bus_client)(sd_bus*, int) = nullptr;
    int (*start)(sd_bus*) = nullptr;
    int (*set_timeout)(sd_bus*, std::uint64_t) = nullptr;
    sd_bus* (*unref)(sd_bus*) = nullptr;
    int (*process)(sd_bus*, sd_bus_message**) = nullptr;
    int (*get_unique_name)(sd_bus*, const char**) = nullptr;
    int (*call_method)(sd_bus*, const char*, const char*, const char*, const char*, sd_bus_error*, sd_bus_message**, const char*, ...) = nullptr;
    int (*call)(sd_bus*, sd_bus_message*, std::uint64_t, sd_bus_error*, sd_bus_message**) = nullptr;
    int (*new_call)(sd_bus*, sd_bus_message**, const char*, const char*, const char*, const char*) = nullptr;
    int (*new_reply)(sd_bus_message*, sd_bus_message**) = nullptr;
    int (*new_signal)(sd_bus*, sd_bus_message**, const char*, const char*, const char*) = nullptr;
    int (*send)(sd_bus*, sd_bus_message*, std::uint64_t*) = nullptr;
    int (*append)(sd_bus_message*, const char*, ...) = nullptr;
    int (*read)(sd_bus_message*, const char*, ...) = nullptr;
    int (*open_container)(sd_bus_message*, char, const char*) = nullptr;
    int (*close_container)(sd_bus_message*) = nullptr;
    int (*enter_container)(sd_bus_message*, char, const char*) = nullptr;
    int (*exit_container)(sd_bus_message*) = nullptr;
    int (*skip)(sd_bus_message*, const char*) = nullptr;
    int (*peek_type)(sd_bus_message*, char*, const char**) = nullptr;
    int (*add_filter)(sd_bus*, sd_bus_slot**, bus_handler, void*) = nullptr;
    int (*add_match)(sd_bus*, sd_bus_slot**, const char*, bus_handler, void*) = nullptr;
    sd_bus_slot* (*slot_unref)(sd_bus_slot*) = nullptr;
    sd_bus_message* (*message_unref)(sd_bus_message*) = nullptr;
    const char* (*get_path)(sd_bus_message*) = nullptr;
    const char* (*get_interface)(sd_bus_message*) = nullptr;
    const char* (*get_member)(sd_bus_message*) = nullptr;
    int (*get_type)(sd_bus_message*, std::uint8_t*) = nullptr;
    int (*reply_error)(sd_bus_message*, const char*, const char*, ...) = nullptr;
};

struct bus_message
{
    bus_api& api;
    sd_bus_message* value = nullptr;
    ~bus_message();
};

class system_bus
{
public:
    explicit system_bus(const std::string& address = {});
    ~system_bus();
    system_bus(const system_bus&) = delete;
    system_bus& operator=(const system_bus&) = delete;
    bus_api api;
    sd_bus* value = nullptr;
    std::string unique_name() const;
    void poll();
};
void bus_check(int result);
}
