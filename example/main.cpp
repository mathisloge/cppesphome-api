#include <print>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <cppesphomeapi/api_client.hpp>

namespace asio = boost::asio;

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
namespace this_coro = asio::this_coro;

namespace
{
awaitable<void> client()
{
    auto executor = co_await this_coro::executor;
    cppesphomeapi::ApiClient api_client{executor, "192.168.0.31"};
    const auto connect_response = co_await api_client.connect();
    if (not connect_response.has_value())
    {
        std::println("Could not connect to client.");
    }
    else
    {
        std::println("Connected to client.");
    }
}

} // namespace
int main()
{
    try
    {
        asio::io_context io_context(1);

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        co_spawn(io_context, client(), detached);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::printf("Exception: %s\n", e.what());
    }
}
