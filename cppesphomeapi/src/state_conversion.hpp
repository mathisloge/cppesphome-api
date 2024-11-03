#pragma once
#include "api.pb.h"
#include "cppesphomeapi/state.hpp"
namespace cppesphomeapi
{
LightState pb2state(const proto::LightStateResponse &response);
}
