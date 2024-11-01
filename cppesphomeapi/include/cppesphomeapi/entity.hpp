#ifndef CPPESPHOMEAPI_ENTITY_HPP
#define CPPESPHOMEAPI_ENTITY_HPP
#include <cstdint>
#include <string>
#include <vector>

namespace cppesphomeapi
{
enum class EntityCategory
{
    None,
    Config,
    Diagnostic
};

enum class ColorMode
{
    Unknown,
    OnOff,
    Brightness,
    White,
    ColorTemperature,
    ColdWarmWhite,
    Rgb,
    RgbWhite,
    RgbColorTemperature,
    RgbColdWarmWhite,
};

struct EntityInfo
{
    std::string object_id;
    std::uint32_t key{};
    std::string name;
    std::string unique_id;
    bool disabled_by_default{};
    std::string icon;
    EntityCategory category{};
};

struct LightEntityInfo : public EntityInfo
{
    float min_mireds;
    float max_mireds;
    std::vector<ColorMode> supported_color_modes;
    std::vector<std::string> effects;
};

struct UserService
{};
} // namespace cppesphomeapi
#endif
