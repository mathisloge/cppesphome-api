#include <print>
#include <thread>
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
    const auto connect_response = co_await api_client.async_connect();
    if (not connect_response.has_value())
    {
        std::println("Could not connect to client.");
        co_return;
    }
    else
    {
        std::println("Connected to client '{}' with version '{}.{}'",
                     api_client.device_name(),
                     api_client.api_version()->major,
                     api_client.api_version()->minor);
    }
    const auto device_info = co_await api_client.async_device_info();
    std::println("Hello dev info: {}, compile_time {}", device_info->name, device_info->compilation_time);

    const auto list = co_await api_client.async_list_entities_services();
    if (not list.has_value())
    {
        std::println("couldn't get list {}", list.error().message);
    }

    co_await api_client.async_light_command({.key = 1111582032, .effect = "Pulsate"});

    co_await api_client.async_disconnect();
}

} // namespace
int main()
{
    try
    {
        asio::io_context io_context(4);

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        co_spawn(io_context, client(), detached);

        std::vector<std::jthread> io_threads;
        for (int i = 0; i < 3; i++)
        {
            io_threads.emplace_back([&io_context]() { io_context.run(); });
        }
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::printf("Exception: %s\n", e.what());
    }
}
