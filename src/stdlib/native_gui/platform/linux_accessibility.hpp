#pragma once

#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/platform/linux_bus.hpp"

namespace tx_generated::native_gui
{
std::string atspi_introspection(const std::vector<std::string>& supported);
class atspi_bridge final : public accessibility_bridge
{
public:
    explicit atspi_bridge(window& owner);
    ~atspi_bridge() override;
    void poll(bool changed) override;
    std::shared_ptr<accessibility_endpoint> endpoint;
    std::unique_ptr<tx::ui::system_bus> bus;
    std::string name, address, desktop_name, desktop_path;
    std::int32_t application_id = 0;
    accessible_tree tree();
    std::string path(const std::string& id) const;
    void reference(sd_bus_message* reply, const std::string& id);
    std::vector<std::string> interfaces(const accessible_node& value) const;
    bool properties(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const accessible_tree& tree, const std::string& method);
    bool methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const accessible_tree& tree, const std::string& interface_name, const std::string& method);
    bool text_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const std::string& interface_name, const std::string& method);
    bool editable_text_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const std::string& interface_name, const std::string& method);
    bool text_selection_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const std::string& interface_name, const std::string& method);
    bool text_geometry_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const std::string& interface_name, const std::string& method);
    bool accessible_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const accessible_tree& tree, const std::string& interface_name, const std::string& method);
    bool component_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const accessible_tree& tree, const std::string& interface_name, const std::string& method);
    bool selection_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
        const accessible_tree& tree, const std::string& interface_name, const std::string& method);
    void signal(const accessible_node& value, const char* event, const std::string& detail,
        int first = 0, int second = 0, const std::string& text = "");
private:
    sd_bus_slot* filter_ = nullptr;
    accessible_tree previous_;
    bool observed_ = false;
    static int dispatch(sd_bus_message* message, void* userdata, sd_bus_error* error);
    bool property(sd_bus_message* reply, const accessible_node& value, const accessible_tree& tree,
        const std::string& interface_name, const std::string& name, bool dictionary);
};
}
