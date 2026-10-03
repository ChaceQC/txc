#include "stdlib/native_gui/platform/linux_bus.hpp"

#include <dlfcn.h>
#include <stdexcept>

namespace tx::ui
{
bus_api::bus_api()
{
    library = dlopen("libsystemd.so.0", RTLD_NOW | RTLD_LOCAL);
    if (!library)
    {
        throw std::runtime_error("无法加载系统 D-Bus 传输 libsystemd.so.0");
    }
    try
    {
#define tx_bus_load(member, symbol) \
        member = reinterpret_cast<decltype(member)>(dlsym(library, symbol)); \
        if (!member) \
        { \
            throw std::runtime_error("系统 D-Bus 缺少接口 " symbol); \
        }
        tx_bus_load(open_user, "sd_bus_open_user")
        tx_bus_load(create, "sd_bus_new")
        tx_bus_load(set_address, "sd_bus_set_address")
        tx_bus_load(set_bus_client, "sd_bus_set_bus_client")
        tx_bus_load(start, "sd_bus_start")
        tx_bus_load(set_timeout, "sd_bus_set_method_call_timeout")
        tx_bus_load(unref, "sd_bus_unref")
        tx_bus_load(process, "sd_bus_process")
        tx_bus_load(get_unique_name, "sd_bus_get_unique_name")
        tx_bus_load(call_method, "sd_bus_call_method")
        tx_bus_load(call, "sd_bus_call")
        tx_bus_load(new_call, "sd_bus_message_new_method_call")
        tx_bus_load(new_reply, "sd_bus_message_new_method_return")
        tx_bus_load(new_signal, "sd_bus_message_new_signal")
        tx_bus_load(send, "sd_bus_send")
        tx_bus_load(append, "sd_bus_message_append")
        tx_bus_load(read, "sd_bus_message_read")
        tx_bus_load(open_container, "sd_bus_message_open_container")
        tx_bus_load(close_container, "sd_bus_message_close_container")
        tx_bus_load(enter_container, "sd_bus_message_enter_container")
        tx_bus_load(exit_container, "sd_bus_message_exit_container")
        tx_bus_load(skip, "sd_bus_message_skip")
        tx_bus_load(peek_type, "sd_bus_message_peek_type")
        tx_bus_load(add_filter, "sd_bus_add_filter")
        tx_bus_load(add_match, "sd_bus_add_match")
        tx_bus_load(slot_unref, "sd_bus_slot_unref")
        tx_bus_load(message_unref, "sd_bus_message_unref")
        tx_bus_load(get_path, "sd_bus_message_get_path")
        tx_bus_load(get_interface, "sd_bus_message_get_interface")
        tx_bus_load(get_member, "sd_bus_message_get_member")
        tx_bus_load(get_type, "sd_bus_message_get_type")
        tx_bus_load(reply_error, "sd_bus_reply_method_errorf")
#undef tx_bus_load
    }
    catch (...)
    {
        dlclose(library);
        throw;
    }
}

bus_api::~bus_api()
{
    dlclose(library);
}

void bus_check(int result)
{
    if (result < 0)
    {
        throw std::runtime_error("系统 D-Bus 操作失败，错误码=" + std::to_string(-result));
    }
}

system_bus::system_bus(const std::string& address)
{
    try
    {
        if (address.empty())
        {
            bus_check(api.open_user(&value));
        }
        else
        {
            bus_check(api.create(&value));
            bus_check(api.set_address(value, address.c_str()));
            bus_check(api.set_bus_client(value, 1));
            bus_check(api.start(value));
        }
    }
    catch (...)
    {
        api.unref(value);
        throw;
    }
    bus_check(api.set_timeout(value, 2000000));
}

system_bus::~system_bus()
{
    api.unref(value);
}

std::string system_bus::unique_name() const
{
    const char* name = nullptr;
    bus_check(api.get_unique_name(value, &name));
    return name ? name : "";
}

void system_bus::poll()
{
    for (unsigned count = 0; count < 128; ++count)
    {
        const auto result = api.process(value, nullptr);
        bus_check(result);
        if (!result)
        {
            break;
        }
    }
}

bus_message::~bus_message()
{
    api.message_unref(value);
}
}
