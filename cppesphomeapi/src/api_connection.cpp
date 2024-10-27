#include "api_connection.hpp"
#include <print>
#include "api.pb.h"
#include "make_unexpected_result.hpp"

namespace asio = boost::asio;
namespace this_coro = asio::this_coro;

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

    auto send_result = co_await send_message_hello();

    co_return send_result;
}

AsyncResult<void> ApiConnection::send_message_hello()
{
    cppesphomeapi::proto::HelloRequest hello_request;

    hello_request.ParseFromArray(nullptr, 0);
    hello_request.set_client_info(std::string{"cppapi"});
    co_await send_message(hello_request);
    const auto msg_promise = co_await receive_message<cppesphomeapi::proto::HelloResponse>();

    std::println("Got esphome device \"{}\": Version {}.{}",
                 msg_promise->name(),
                 msg_promise->api_version_major(),
                 msg_promise->api_version_minor());
    // todo: do something with the message.
    co_return Result<void>{};
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
} // namespace cppesphomeapi
