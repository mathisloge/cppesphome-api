#include "api_connection.hpp"
#include <print>
#include <boost/asio.hpp>
#include "api.pb.h"
#include "make_unexpected_result.hpp"

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

    asio::co_spawn(strand_, std::bind(&ApiConnection::async_receive, this), asio::detached);

    REQUIRE_SUCCESS(co_await send_message_hello());
    REQUIRE_SUCCESS(co_await send_message_connect());

    asio::co_spawn(executor, std::bind(&ApiConnection::subscribe_logs, this), asio::detached);
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
    auto response = co_await receive_message<proto::HelloResponse>();
    REQUIRE_SUCCESS(response);
    auto message = std::move(response.value());
    device_name_ = message->name();
    api_version_ = ApiVersion{.major = message->api_version_major(), .minor = message->api_version_minor()};
    co_return Result<void>{};
}

AsyncResult<void> ApiConnection::send_message_connect()
{
    proto::ConnectRequest request;
    request.set_password(password_);
    REQUIRE_SUCCESS(co_await send_message(request));
    auto response = co_await receive_message<proto::ConnectResponse>();
    REQUIRE_SUCCESS(response);
    auto message = std::move(response.value());
    if (message->invalid_password())
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
    auto message = std::move(response.value());

    co_return DeviceInfo{
        .uses_password = message->uses_password(),
        .has_deep_sleep = message->has_deep_sleep(),
        .name = message->name(),
        .friendly_name = message->friendly_name(),
        .mac_address = message->mac_address(),
        .compilation_time = message->compilation_time(), // todo: maybe parse directly into std::chrono?
        .model = message->model(),
        .manufacturer = message->manufacturer(),
        .esphome_version = message->esphome_version(),
        .webserver_port = static_cast<uint16_t>(message->webserver_port()),
        .suggested_area = message->suggested_area(),
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
        std::println("GOT LIST .{}", std::visit([](auto &&msg) { return msg->key(); }, msg));
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
    const auto packet = PlainTextProtocol::serialize(message);
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

boost::asio::awaitable<void> ApiConnection::subscribe_logs()
{
    proto::SubscribeLogsRequest request;
    request.set_dump_config(true);
    request.set_level(::cppesphomeapi::proto::LogLevel::LOG_LEVEL_VERY_VERBOSE);
    co_await send_message(request);
    while (true)
    {
        auto response = co_await receive_message<proto::SubscribeLogsResponse>();
        if (response.has_value())
        {
            std::println("Got log message {}", response.value()->message());
        }
    }
}

boost::asio::awaitable<void> ApiConnection::async_receive()
{
    namespace asio = boost::asio;
    std::array<std::uint8_t, 2048> buffer{};
    bool do_receive{true};

    while (do_receive)
    {
        const auto received_bytes = co_await socket_.async_receive(asio::buffer(buffer));

        auto result = PlainTextProtocol{}
                          .decode_multiple<proto::SubscribeLogsResponse,
                                           proto::DeviceInfoResponse,
                                           proto::ConnectResponse,
                                           proto::HelloResponse,
                                           proto::DisconnectResponse,
                                           proto::ListEntitiesDoneResponse,
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
                                           proto::ListEntitiesValveResponse>(
                              std::span{buffer.begin(), received_bytes}, [this](auto &&message) {
                                  std::vector<boost::asio::any_completion_handler<void(
                                      std::shared_ptr<google::protobuf::Message> message)>>
                                      handlers;
                                  {
                                      std::unique_lock l{handler_mtx_};
                                      handlers = std::exchange(handlers_, {});
                                  }
                                  // todo: add small ring buffer if the handlers are empty or try to return the
                                  // acceptance from the handler.
                                  for (auto &&handler : handlers)
                                  {
                                      auto work = boost::asio::make_work_guard(handler);
                                      auto alloc = boost::asio::get_associated_allocator(
                                          handler, boost::asio::recycling_allocator<void>());

                                      // Dispatch the completion handler through the handler's associated
                                      // executor, using the handler's associated allocator.
                                      boost::asio::dispatch(
                                          work.get_executor(),
                                          boost::asio::bind_allocator(
                                              alloc, [handler = std::move(handler), result = message]() mutable {
                                                  std::move(handler)(std::move(result));
                                              }));
                                  }
                                  // std::println("Received message {}", message->GetTypeName());
                              });
    }
    std::println("RECEIVE ENDED!");
}
} // namespace cppesphomeapi
