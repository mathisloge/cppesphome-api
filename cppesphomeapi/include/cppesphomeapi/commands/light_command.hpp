#ifndef CPPESPHOMEAPI_LIGHT_COMMAND_HPP
#define CPPESPHOMEAPI_LIGHT_COMMAND_HPP
#include <cstdint>
#include <optional>
#include <string>

namespace cppesphomeapi
{
struct LightCommand
{
    std::uint32_t key{};
    std::optional<std::string> effect;
};
} // namespace cppesphomeapi

#endif
