#ifndef CPPESPHOMEAPI_STATE_HPP
#define CPPESPHOMEAPI_STATE_HPP
#include <cstdint>
#include "entity.hpp"

namespace cppesphomeapi
{
struct EntityState
{
    std::uint32_t key{};
};

struct LightState : EntityState
{
    bool state{};
    float brightness{};
    ColorMode color_mode{};
    float color_brightness{};
    float red{};
    float green{};
    float blue{};
    float white{};
    float color_temperature{};
    float cold_white{};
    float warm_white{};
    std::string effect;
};
} // namespace cppesphomeapi
#endif
