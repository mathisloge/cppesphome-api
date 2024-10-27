#include "api_connection.hpp"
#include "api.pb.h"
#include "make_unexpected_result.hpp"

#include <print>

namespace asio = boost::asio;
namespace this_coro = asio::this_coro;

#define REQUIRE_SUCCESS(async_operation)                                                                               \
    {                                                                                                                  \
        auto &&result = async_operation;                                                                               \
        if (not result.has_value())                                                                                    \
        {                                                                                                              \
            co_return std::unexpected(result.error());                                                                 \
        }                                                                                                              \
    }

namespace cppesphomeapi
{
ApiConnection::ApiConnection(std::string hostname,
                             std::uint16_t port,
                             std::string password,
                             const asio::any_io_executor &executor)
    : hostname_{std::move(hostname)}
    , port_{port}
    , password_{std::move(password)}
    , strand_{asio::make_strand(executor)}
    , socket_{strand_}
{}

AsyncResult<void> ApiConnection::connect()
{
    auto executor = co_await this_coro::executor;
    tcp::resolver resolver{executor};
    const auto resolved = co_await resolver.async_resolve(hostname_, std::to_string(port_));

    co_await socket_.async_connect(resolved->endpoint());
    socket_.set_option(asio::socket_base::keep_alive{true});

    REQUIRE_SUCCESS(co_await send_message_hello());
    REQUIRE_SUCCESS(co_await send_message_connect());

    co_return Result<void>{};
}

AsyncResult<void> ApiConnection::disconnect()
{
    proto::DisconnectRequest request;
    REQUIRE_SUCCESS(co_await send_message(request));
    REQUIRE_SUCCESS(co_await receive_message<proto::DisconnectResponse>());
    co_return Result<void>{};
}

AsyncResult<void> ApiConnection::send_message_hello()
{
    proto::HelloRequest request;
    request.set_client_info(std::string{"cppapi"});
    REQUIRE_SUCCESS(co_await send_message(request));
    const auto response = co_await receive_message<proto::HelloResponse>();
    REQUIRE_SUCCESS(response);
    device_name_ = response->name();
    api_version_ = ApiVersion{.major = response->api_version_major(), .minor = response->api_version_minor()};
    co_return Result<void>{};
}

AsyncResult<void> ApiConnection::send_message_connect()
{
    proto::ConnectRequest request;
    request.set_password(password_);
    REQUIRE_SUCCESS(co_await send_message(request));
    const auto response = co_await receive_message<proto::ConnectResponse>();
    if (response->invalid_password())
    {
        co_return make_unexpected_result(ApiErrorCode::AuthentificationError, "Invalid password");
    }
    co_return Result<void>{};
}

AsyncResult<DeviceInfo> ApiConnection::request_device_info()
{
    proto::DeviceInfoRequest device_request{};
    REQUIRE_SUCCESS(co_await send_message(device_request));
    const auto response = co_await receive_message<proto::DeviceInfoResponse>();
    REQUIRE_SUCCESS(response);

    co_return DeviceInfo{
        .uses_password = response->uses_password(),
        .has_deep_sleep = response->has_deep_sleep(),
        .name = response->name(),
        .friendly_name = response->friendly_name(),
        .mac_address = response->mac_address(),
        .compilation_time = response->compilation_time(), // todo: maybe parse directly into std::chrono?
        .model = response->model(),
        .manufacturer = response->manufacturer(),
        .esphome_version = response->esphome_version(),
        .webserver_port = static_cast<uint16_t>(response->webserver_port()),
        .suggested_area = response->suggested_area(),
    };
}

AsyncResult<std::vector<EntityInfo>> ApiConnection::request_entities_and_services()
{
    proto::ListEntitiesRequest request;
    REQUIRE_SUCCESS(co_await send_message(request));
    const auto messages = co_await receive_messages<proto::ListEntitiesDoneResponse,
                                                    proto::ListEntitiesAlarmControlPanelResponse,
                                                    proto::ListEntitiesBinarySensorResponse,
                                                    proto::ListEntitiesButtonResponse,
                                                    proto::ListEntitiesCameraResponse,
                                                    proto::ListEntitiesClimateResponse,
                                                    proto::ListEntitiesCoverResponse,
                                                    proto::ListEntitiesDateResponse,
                                                    proto::ListEntitiesDateTimeResponse,
                                                    proto::ListEntitiesEventResponse,
                                                    proto::ListEntitiesFanResponse,
                                                    proto::ListEntitiesLightResponse,
                                                    proto::ListEntitiesLockResponse,
                                                    proto::ListEntitiesMediaPlayerResponse,
                                                    proto::ListEntitiesNumberResponse,
                                                    proto::ListEntitiesSelectResponse,
                                                    proto::ListEntitiesSensorResponse,
                                                    proto::ListEntitiesServicesResponse,
                                                    proto::ListEntitiesSwitchResponse,
                                                    proto::ListEntitiesTextResponse,
                                                    proto::ListEntitiesTextSensorResponse,
                                                    proto::ListEntitiesTimeResponse,
                                                    proto::ListEntitiesUpdateResponse,
                                                    proto::ListEntitiesValveResponse>();
    REQUIRE_SUCCESS(messages);

    for (auto &&msg : messages.value())
    {
        std::println("GOT LIST .{}", std::visit([](auto &&msg) { return msg.key(); }, msg));
    }
    co_return std::vector<EntityInfo>{};
}

AsyncResult<void> ApiConnection::light_command(LightCommand light_command)
{
    proto::LightCommandRequest request{};
    request.set_key(light_command.key);

    if (light_command.effect.has_value())
    {
        request.set_effect(std::move(light_command.effect.value()));
    }
    co_return co_await send_message(request);
}

AsyncResult<void> ApiConnection::send_message(const google::protobuf::Message &message)
{
    const auto packet = plain_text_serialize(message);
    if (packet.has_value())
    {
        const auto written = co_await socket_.async_write_some(asio::buffer(packet.value()));
        if (written != packet->size())
        {
            co_return make_unexpected_result(
                ApiErrorCode::SendError,
                std::format("Could not send message. Bytes written are different: expected={}, written={}",
                            packet->size(),
                            written));
        }
        co_return Result<void>{};
    }
    co_return std::unexpected(packet.error());
}

const std::optional<ApiVersion> &ApiConnection::api_version() const
{
    return api_version_;
}

const std::string &ApiConnection::device_name() const
{
    return device_name_;
}
} // namespace cppesphomeapi
