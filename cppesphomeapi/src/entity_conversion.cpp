#include "entity_conversion.hpp"
#include <cstdint>

namespace cppesphomeapi
{
template <typename T>
concept EntityAlikeMessage = requires(T msg) {
    { msg.object_id() } -> std::convertible_to<std::string>;
    { msg.key() } -> std::same_as<std::uint32_t>;
    { msg.name() } -> std::convertible_to<std::string>;
    { msg.unique_id() } -> std::convertible_to<std::string>;
    { msg.disabled_by_default() } -> std::same_as<bool>;
    { msg.icon() } -> std::convertible_to<std::string>;
    { msg.entity_category() } -> std::convertible_to<proto::EntityCategory>;
};

EntityInfo pb2entity_info_base(const EntityAlikeMessage auto &entity)
{
    constexpr auto kMapEntityCategory = [](proto::EntityCategory category) -> EntityCategory {
        switch (category)
        {
        case proto::EntityCategory::ENTITY_CATEGORY_NONE:
            return EntityCategory::None;
        case proto::EntityCategory::ENTITY_CATEGORY_CONFIG:
            return EntityCategory::Config;
        case proto::EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC:
            return EntityCategory::Diagnostic;
        // protobuf won't use these as return param but handle them to get warnings if new flags are added.
        case proto::EntityCategory::EntityCategory_INT_MIN_SENTINEL_DO_NOT_USE_:
        case proto::EntityCategory::EntityCategory_INT_MAX_SENTINEL_DO_NOT_USE_:
            break;
        }
        return EntityCategory::None;
    };
    return EntityInfo{
        .object_id = entity.object_id(),
        .key = entity.key(),
        .name = entity.name(),
        .unique_id = entity.unique_id(),
        .disabled_by_default = entity.disabled_by_default(),
        .icon = entity.icon(),
        .category = kMapEntityCategory(entity.entity_category()),
    };
}

LightEntityInfo pb2entity_info(const proto::ListEntitiesLightResponse &light_response)
{
    LightEntityInfo entity{
        {pb2entity_info_base(light_response)},
        /* .min_mireds =*/light_response.min_mireds(),
        /* .max_mireds =*/light_response.max_mireds(),
    };

    entity.effects.reserve(light_response.effects_size());
    std::ranges::copy(light_response.effects(), std::back_inserter(entity.effects));

    constexpr auto kMapColorModes = [](int mode) -> ColorMode {
        switch (proto::ColorMode{mode})
        {
        case proto::ColorMode::COLOR_MODE_UNKNOWN:
            return ColorMode::Unknown;
        case proto::ColorMode::COLOR_MODE_ON_OFF:
            return ColorMode::OnOff;
        case proto::ColorMode::COLOR_MODE_BRIGHTNESS:
            return ColorMode::Brightness;
        case proto::ColorMode::COLOR_MODE_WHITE:
            return ColorMode::White;
        case proto::ColorMode::COLOR_MODE_COLOR_TEMPERATURE:
            return ColorMode::ColorTemperature;
        case proto::ColorMode::COLOR_MODE_COLD_WARM_WHITE:
            return ColorMode::ColdWarmWhite;
        case proto::ColorMode::COLOR_MODE_RGB:
            return ColorMode::Rgb;
        case proto::ColorMode::COLOR_MODE_RGB_WHITE:
            return ColorMode::RgbWhite;
        case proto::ColorMode::COLOR_MODE_RGB_COLOR_TEMPERATURE:
            return ColorMode::RgbColorTemperature;
        case proto::ColorMode::COLOR_MODE_RGB_COLD_WARM_WHITE:
            return ColorMode::RgbColdWarmWhite;
        // protobuf won't use these as return param but handle them to get warnings if new flags are added.
        case proto::ColorMode::ColorMode_INT_MIN_SENTINEL_DO_NOT_USE_:
        case proto::ColorMode::ColorMode_INT_MAX_SENTINEL_DO_NOT_USE_:
            break;
        }
        return ColorMode::Unknown;
    };
    entity.supported_color_modes.reserve(light_response.supported_color_modes_size());
    std::ranges::transform(
        light_response.supported_color_modes(), std::back_inserter(entity.supported_color_modes), kMapColorModes);

    return entity;
}
} // namespace cppesphomeapi
