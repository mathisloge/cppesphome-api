#include "net.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

using namespace boost::asio;
namespace aex = boost::asio::experimental;
using aex::awaitable_operators::operator||;

namespace cppesphomeapi::net
{
using namespace detail;

// precondition: not Data.empty()
auto sendTo(Socket &socket, Timer &timer, ConstByteSpan data) -> asio::awaitable<ExpectSize>
{
    co_return flatten(co_await (socket.async_send(asio::buffer(data)) || timer.async_wait()));
}

// precondition: not buffer.empty()
auto receiveFrom(Socket &socket, Timer &timer, ByteSpan byte_buffer) -> asio::awaitable<ExpectSize>
{
    co_return flatten(co_await (socket.async_receive(buffer(byte_buffer)) || timer.async_wait()));
}

// precondition: not Endpoints.empty()
auto connectTo(Endpoints endpoints, Timer &timer) -> asio::awaitable<ExpectSocket>
{
    Socket socket(timer.get_executor());
    co_return replace(flatten(co_await (async_connect(socket, endpoints) || timer.async_wait())), std::move(socket));
}

auto expired(Timer &timer) noexcept -> asio::awaitable<bool>
{
    const auto [Error] = co_await timer.async_wait();
    co_return not Error;
}

void close(Socket &socket) noexcept
{
    ErrorCode error;
    std::ignore = socket.shutdown(Socket::shutdown_both, error);
    std::ignore = socket.close(error);
}

auto resolveHostEndpoints(std::string_view HostName, Port port, Timer &timer)
    -> asio::awaitable<Expected<std::vector<Endpoint>>>
{
    using namespace std::string_view_literals;
    static constexpr auto kLocalHostName = "localhost"sv;

    using Resolver = use_await::as_default_on_t<asio::ip::tcp::resolver>;
    auto flags = Resolver::numeric_service;
    if (HostName.empty() || HostName == kLocalHostName)
    {
        flags |= Resolver::passive;
        HostName = kLocalHostName;
    }

    auto executor = co_await this_coro::executor;
    Resolver resolver{executor};
    const auto resolved =
        flatten(co_await (resolver.async_resolve(HostName, std::to_string(port), flags) || timer.async_wait()));
    if (not resolved.has_value())
    {
        co_return std::unexpected{resolved.error()};
    }

    std::vector<Endpoint> endpoints;
    endpoints.reserve(resolved->size());
    for (const auto &element : *resolved)
    {
        endpoints.emplace_back(element.endpoint());
    }
    co_return endpoints;
}
} // namespace cppesphomeapi::net
