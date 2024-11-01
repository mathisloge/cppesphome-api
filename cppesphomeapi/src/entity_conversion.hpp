#pragma once
#include "api.pb.h"
#include "cppesphomeapi/entity.hpp"

namespace cppesphomeapi
{
LightEntityInfo pb2entity_info(const proto::ListEntitiesLightResponse &light_response);
} // namespace cppesphomeapi
