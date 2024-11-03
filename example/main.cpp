#include <print>
#include <thread>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
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
    std::stop_source stop_source;
    auto executor = co_await this_coro::executor;
    std::println("client running on thread {}", std::this_thread::get_id());
    cppesphomeapi::ApiClient api_client{executor, stop_source, "192.168.0.31"};
    const auto connect_response = co_await api_client.async_connect();
    if (not connect_response.has_value())
    {
        std::println("Could not connect to client. {}", connect_response.error().message);
        co_return;
    }
    else
    {
        std::println("Connected to client '{}' with version '{}.{}'",
                     api_client.device_name(),
                     api_client.api_version()->major,
                     api_client.api_version()->minor);
    }

    co_spawn(
        executor,
        [&api_client]() -> asio::awaitable<void> {
            while (true)
            {
                auto log_message = co_await api_client.async_receive_log();
                if (log_message.has_value())
                {
                    std::println("ESPHOME_LOG: {}", log_message->message);
                }
            }
        },
        detached);
    co_spawn(
        executor,
        [&api_client]() -> asio::awaitable<void> {
            while (true)
            {
                auto state_message = co_await api_client.async_receive_state();
                if (state_message.has_value())
                {
                    std::println("ESPHOME_STATE: {}", "");
                }
            }
        },
        detached);
    co_await api_client.subscribe_logs(cppesphomeapi::EspHomeLogLevel::VeryVerbose, true);
    co_await api_client.subscribe_states();

    const auto device_info = co_await api_client.async_device_info();
    std::println("Hello dev info: {}, compile_time {}", device_info->name, device_info->compilation_time);

    const auto list = co_await api_client.async_list_entities_services();
    if (not list.has_value())
    {
        std::println("couldn't get list {}", list.error().message);
    }
    else
    {
        for (auto &&entity : list.value())
        {
            std::visit([](auto &&msg) { std::println("Got entity {}", msg.name); }, entity);
        }
    }
    co_await api_client.async_light_command({.key = 1111582032, .effect = "Pulsate"});
    asio::steady_timer timer{executor, std::chrono::seconds{10}};
    co_await timer.async_wait();
    co_await api_client.async_disconnect();
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

        std::jthread quit_thread{[&]() {
            std::this_thread::sleep_for(std::chrono::seconds{15});
            std::println("stopping");
            io_context.stop();
        }};
        // std::vector<std::jthread> io_threads;
        // for (int i = 0; i < 3; i++)
        // {
        //     io_threads.emplace_back([&io_context]() { io_context.run(); });
        // }
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::printf("Exception: %s\n", e.what());
    }
}
