#pragma once
#include "api.pb.h"
#include "cppesphomeapi/entity.hpp"

namespace cppesphomeapi
{
EntityInfo pb2entity_info(const proto::ListEntitiesAlarmControlPanelResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesBinarySensorResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesButtonResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesCameraResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesClimateResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesCoverResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesDateResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesDateTimeResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesEventResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesFanResponse &response);
LightEntityInfo pb2entity_info(const proto::ListEntitiesLightResponse &light_response);
EntityInfo pb2entity_info(const proto::ListEntitiesLockResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesMediaPlayerResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesNumberResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesSelectResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesSensorResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesServicesResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesSwitchResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesTextResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesTextSensorResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesTimeResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesUpdateResponse &response);
EntityInfo pb2entity_info(const proto::ListEntitiesValveResponse &response);

ColorMode pb2color_mode(proto::ColorMode color_mode);
} // namespace cppesphomeapi
