#ifndef CPPESPHOMEAPI_DEVICE_INFO_HPP
#define CPPESPHOMEAPI_DEVICE_INFO_HPP
#include <cstdint>
#include <string>

namespace cppesphomeapi
{
struct DeviceInfo
{
    bool uses_password{};
    bool has_deep_sleep{};
    std::string name;
    std::string friendly_name;
    std::string mac_address;
    std::string compilation_time;
    std::string model;
    std::string manufacturer;
    std::string esphome_version;
    std::uint16_t webserver_port;
    std::string suggested_area;
};
} // namespace cppesphomeapi
#endif
