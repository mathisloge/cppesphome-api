#include "api_connection.hpp"
#include <print>
#include <boost/asio.hpp>
#include "api.pb.h"
#include "entity_conversion.hpp"
#include "executor.hpp"
#include "make_unexpected_result.hpp"
#include "net.hpp"
#include "plain_text_protocol.hpp"

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
                             std::stop_source &stop_source,
                             const asio::any_io_executor &executor)
    : hostname_{std::move(hostname)}
    , port_{port}
    , password_{std::move(password)}
    , socket_{executor}
{
    executor::addStopService(executor::getContext(executor), stop_source);
}

void ApiConnection::cancel()
{
    executor::stopAssetOf(executor::getContext(socket_.get_executor())).request_stop();
}

AsyncResult<void> ApiConnection::connect()
{
    auto executor = co_await this_coro::executor;
    net::Timer timer{executor};
    timer.expires_after(std::chrono::milliseconds{500});

    auto endpoints = co_await net::resolveHostEndpoints(hostname_, net::Port{port_}, timer);
    if (not endpoints.has_value())
    {
        co_return make_unexpected_result(
            ApiErrorCode::UnexpectedMessage,
            std::format(
                "Could not resolve host {}:{}. Failed with error: {}", hostname_, port_, endpoints.error().message()));
    }

    timer.expires_after(std::chrono::milliseconds{500});
    auto connect_result = co_await net::connectTo(*endpoints, timer);
    if (not connect_result.has_value())
    {
        co_return make_unexpected_result(ApiErrorCode::UnexpectedMessage,
                                         std::format("Could not connect to host {}:{}. Failed with error: {}",
                                                     hostname_,
                                                     port_,
                                                     endpoints.error().message()));
    }

    socket_ = std::move(*connect_result);

    executor::commission(socket_.get_executor(), &ApiConnection::receive_loop, this);
    executor::commission(socket_.get_executor(), &ApiConnection::heartbeat_loop, this);

    REQUIRE_SUCCESS(co_await send_message_hello());
    REQUIRE_SUCCESS(co_await send_message_connect());
    co_return Result<void>{};
}

AsyncResult<void> ApiConnection::disconnect()
{
    proto::DisconnectRequest request;
    REQUIRE_SUCCESS(co_await send_message(request));
    REQUIRE_SUCCESS(co_await receive_message<proto::DisconnectResponse>(asio::use_awaitable));
    co_return Result<void>{};
}

AsyncResult<void> ApiConnection::send_message_hello()
{
    proto::HelloRequest request;
    request.set_client_info(std::string{"cppapi"});
    REQUIRE_SUCCESS(co_await send_message(request));
    auto response = co_await receive_message<proto::HelloResponse>(asio::use_awaitable);
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
    auto response = co_await receive_message<proto::ConnectResponse>(asio::use_awaitable);
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
    const auto response = co_await receive_message<proto::DeviceInfoResponse>(asio::use_awaitable);
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

AsyncResult<EntityInfoList> ApiConnection::request_entities_and_services()
{
    proto::ListEntitiesRequest request;
    auto message_receiver = receive_messages<proto::ListEntitiesDoneResponse,
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
                                             proto::ListEntitiesValveResponse>(asio::deferred);
    REQUIRE_SUCCESS(co_await send_message(request));
    auto messages = co_await std::move(message_receiver);
    REQUIRE_SUCCESS(messages);

    EntityInfoList list;
    list.reserve(messages->size());

    std::ranges::transform(messages.value(), std::back_inserter(list), [](auto &&msg) {
        return std::visit([](auto &&msg) { return EntityInfoVariant{pb2entity_info(*msg)}; }, msg);
    });
    co_return list;
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
        std::println("Sending {}", message.GetTypeName());
        auto executor = co_await this_coro::executor;
        net::Timer timer{executor};
        const auto watch_dog = executor::abort(socket_, timer);
        timer.expires_after(std::chrono::milliseconds{500});

        const auto written = co_await net::sendTo(socket_, timer, packet.value());
        if (written.has_value() && written.value() != packet->size())
        {
            co_return make_unexpected_result(
                ApiErrorCode::SendError,
                std::format("Could not send message. Bytes written are different: expected={}, written={}",
                            packet->size(),
                            *written));
        }
        else if (not written.has_value())
        {
            co_return make_unexpected_result(
                ApiErrorCode::SendError,
                std::format("Could not send message in time. Error: {}", written.error().message()));
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

AsyncResult<void> ApiConnection::enable_logs(EspHomeLogLevel log_level, bool config_dump)
{
    proto::SubscribeLogsRequest request;
    request.set_dump_config(config_dump);
    request.set_level(::cppesphomeapi::proto::LogLevel(std::to_underlying(log_level)));
    co_return co_await send_message(request);
}

AsyncResult<LogEntry> ApiConnection::receive_log()
{
    const auto message = co_await receive_message<proto::SubscribeLogsResponse>(asio::use_awaitable);
    REQUIRE_SUCCESS(message);
    co_return LogEntry{
        .log_level = EspHomeLogLevel{std::to_underlying(message.value()->level())},
        .message = message.value()->message(),
    };
}

boost::asio::awaitable<void> ApiConnection::receive_loop()
{
    std::array<std::byte, 4096> buffer{};
    auto executor = co_await this_coro::executor;
    net::Timer timer{executor};
    const auto watch_dog = executor::abort(socket_, timer);

    bool do_receive{true};
    while (do_receive)
    {
        timer.expires_after(std::chrono::seconds{100}); // at least every 90secs. a message should be received
        const auto received_bytes = co_await net::receiveFrom(socket_, timer, buffer);
        if (not received_bytes.has_value())
        {
            std::println("Could not receive bytes. Error {}", received_bytes.error().message());
            break;
        }
        auto result = PlainTextProtocol{}
                          .decode_multiple<proto::SubscribeLogsResponse,
                                           proto::DeviceInfoResponse,
                                           proto::ConnectResponse,
                                           proto::HelloResponse,
                                           proto::PingResponse,
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
                              std::span{buffer.cbegin(), received_bytes.value()}, [this](auto &&message) {
                                  std::vector<boost::asio::any_completion_handler<void(MessageWrapper)>> handlers;
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
                                  std::println("Received message {}", message.ref().GetTypeName());
                              });
    }
    std::println("RECEIVE ENDED!");
}

boost::asio::awaitable<void> ApiConnection::heartbeat_loop()
{
    auto executor = co_await this_coro::executor;
    asio::steady_timer timer{executor};

    while (true)
    {
        timer.expires_after(std::chrono::seconds{20});
        co_await timer.async_wait();
        proto::PingRequest request;
        co_await send_message(request);
        // todo: this should be in a different coroutine and only expect a response at the minimum of 20sec*4.5
        // otherwise the connection is dead.
        co_await receive_message<proto::PingResponse>(asio::use_awaitable);
    }
    executor::stopAssetOf(executor).request_stop();
}

} // namespace cppesphomeapi
