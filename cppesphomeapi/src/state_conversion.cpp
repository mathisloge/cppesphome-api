#include "state_conversion.hpp"
#include "entity_conversion.hpp"
namespace cppesphomeapi
{
LightState pb2state(const proto::LightStateResponse &response)
{
    return LightState{
        {response.key()},
        /*.state =*/response.state(),
        /*.brightness =*/response.brightness(),
        /*.color_mode =*/pb2color_mode(response.color_mode()),
        /*.color_brightness=*/response.color_brightness(),
        /*.red =*/response.red(),
        /*.green =*/response.green(),
        /*.blue =*/response.blue(),
        /*.white =*/response.white(),
        /*.color_temperature =*/response.color_temperature(),
        /*.cold_white =*/response.cold_white(),
        /*.warm_white =*/response.warm_white(),
        /*.effect =*/response.effect(),
    };
}
} // namespace cppesphomeapi
