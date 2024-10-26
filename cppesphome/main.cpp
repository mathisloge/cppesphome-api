#include <cstdio>
#include <print>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include "api.pb.h"
#include "api_options.pb.h"

namespace asio = boost::asio;

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::use_awaitable_t;
using asio::ip::tcp;
using tcp_acceptor = use_awaitable_t<>::as_default_on_t<tcp::acceptor>;
using tcp_socket = use_awaitable_t<>::as_default_on_t<tcp::socket>;
namespace this_coro = asio::this_coro;

uint8_t varuint_to_bytes(std::uint32_t value, std::span<std::uint8_t> data)
{
    if (value <= 0x7F)
    {
        data[0] = static_cast<uint8_t>(value);
        return 1;
    }

    std::uint32_t write_index{0};
    while (value != 0)
    {
        const uint8_t temp = value & 0x7F;
        if (value != 0)
        {
            data[write_index] = (temp | 0x80);
        }
        else
        {
            data[write_index] = temp;
        }
        value >>= 7;
        write_index++;
    }
    return write_index;
}

std::optional<std::uint32_t> read_varuint(auto &iterator, const auto end)
{
    std::uint32_t result = 0;
    std::uint32_t bitpos = 0;
    while (iterator != end)
    {
        const auto val = *iterator;
        iterator++;
        result |= (val & 0x7F) << bitpos;
        if ((val & 0x80) == 0)
        {
            return result;
        }
        bitpos += 7;
    }
    return std::nullopt; // Buffer ran out of bytes
}

awaitable<std::uint32_t> write_message(const ::google::protobuf::Message &message, tcp::socket &socket)
{
    auto executor = co_await this_coro::executor;
    const auto msg_size = message.ByteSizeLong();
    auto &&msg_options = message.GetDescriptor()->options();
    if (not msg_options.HasExtension(id))
    {
        std::println("message does not have an id");
        co_return 0;
    }

    std::array<std::uint8_t, 256> buffer{};
    std::uint32_t current_write_index{0};
    buffer.at(current_write_index++) = 0;
    current_write_index += varuint_to_bytes(msg_size, {&buffer.at(current_write_index), buffer.end()});
    current_write_index +=
        varuint_to_bytes(msg_options.GetExtension(id), {&buffer.at(current_write_index), buffer.end()});
    const auto header_len = current_write_index;
    message.SerializeToArray(&buffer.at(current_write_index), buffer.size() - current_write_index);

    const auto written = co_await socket.async_write_some(asio::buffer(buffer, msg_size + current_write_index));
    std::println(
        "written = {}, expected = {}, header len={}", written, (msg_size + current_write_index), current_write_index);
    co_return written;
}

awaitable<void> receive_response(tcp::socket &socket, auto &received_message)
{
    std::array<std::uint8_t, 512> buffer{};
    const auto received_bytes = co_await socket.async_receive(asio::buffer(buffer));
    std::println("received {} bytes", received_bytes);
    if (received_bytes < 3)
    {
        std::println("response does not contain enough bytes for the header");
        co_return;
    }

    auto *it = buffer.begin();
    const auto preamble = read_varuint(it, buffer.end());
    if (not preamble.has_value() or *preamble != 0x00)
    {
        std::println("response does contain an invalid preamble");
        co_return;
    }
    const auto message_size = read_varuint(it, buffer.end());
    if (not message_size.has_value())
    {
        std::println("got no message size");
        co_return;
    }
    const auto message_type = read_varuint(it, buffer.end());
    if (not message_type.has_value())
    {
        std::println("got no message type");
        co_return;
    }
    const auto expected_message_id = HelloResponse::GetDescriptor()->options().GetExtension(id);
    if (expected_message_id != message_type.value())
    {
        std::println("Expected {} but got message id {}", HelloResponse::GetDescriptor()->name(), message_type.value());
        co_return;
    }
    received_message.ParseFromArray(it, message_size.value());
    std::println("received message: {}, {}", message_size.value(), message_type.value());
}

awaitable<void> send_message_hello(tcp::socket &socket)
{
    std::println("send hello");
    HelloRequest hello_request;
    hello_request.set_client_info(std::string{"cppapi"});
    co_await write_message(hello_request, socket);
    HelloResponse response;
    co_await receive_response<HelloResponse>(socket, response);

    std::println("Got esphome device \"{}\": Version {}.{}",
                 response.name(),
                 response.api_version_major(),
                 response.api_version_minor());
}

awaitable<void> client()
{
    std::println("client launched");
    auto executor = co_await this_coro::executor;
    tcp::socket socket{executor};
    tcp::resolver resolver{executor};
    std::println("trying to resolve client");
    const auto resolved = co_await resolver.async_resolve("192.168.0.31", "6053");

    std::println("client resolved");

    co_await socket.async_connect(resolved->endpoint());
    socket.set_option(asio::socket_base::keep_alive{true});
    std::println("connected");
    co_await send_message_hello(socket);
}

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
