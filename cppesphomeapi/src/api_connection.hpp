#pragma once
#include <cstdint>
#include <string>
#include <boost/asio/executor.hpp>
#include <boost/asio/strand.hpp>
#include <google/protobuf/message.h>
#include "cppesphomeapi/api_client.hpp"
#include "cppesphomeapi/api_version.hpp"
#include "cppesphomeapi/async_result.hpp"
#include "cppesphomeapi/commands.hpp"
#include "cppesphomeapi/device_info.hpp"
#include "plain_text_protocol.hpp"
#include "tcp.hpp"

namespace cppesphomeapi
{
namespace detail
{
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
} // namespace detail

class ApiConnection
{
  public:
    explicit ApiConnection(std::string hostname,
                           std::uint16_t port,
                           std::string password,
                           const boost::asio::any_io_executor &executor);

    AsyncResult<void> connect();
    AsyncResult<void> disconnect();
    AsyncResult<void> send_message_hello();
    AsyncResult<void> send_message_connect();
    AsyncResult<DeviceInfo> request_device_info();
    AsyncResult<std::vector<EntityInfo>> request_entities_and_services();
    AsyncResult<void> light_command(LightCommand light_command);
    const std::optional<ApiVersion> &api_version() const;
    const std::string &device_name() const;

  private:
    AsyncResult<void> send_message(const google::protobuf::Message &message);

    template <typename TMsg>
    auto receive_message() -> AsyncResult<TMsg>
    {
        namespace asio = boost::asio;
        std::array<std::uint8_t, 512> buffer{};
        const auto received_bytes = co_await socket_.async_receive(asio::buffer(buffer));
        co_return plain_text_decode<TMsg>(std::span{buffer.begin(), received_bytes});
    }

    template <typename TStopMsg, typename... TMsgs>
    auto receive_messages() -> AsyncResult<std::vector<std::variant<TMsgs...>>>
    {
        namespace asio = boost::asio;
        std::vector<std::variant<TMsgs...>> messages;
        std::array<std::uint8_t, 2048> buffer{};

        // todo: add stop source
        bool do_receive{true};
        while (do_receive)
        {
            const auto received_bytes = co_await socket_.async_receive(asio::buffer(buffer));
            auto multiple_messages =
                plain_text_decode_multiple<TStopMsg, TMsgs...>(std::span{buffer.begin(), received_bytes});
            if (not multiple_messages.has_value())
            {
                co_return std::unexpected(multiple_messages.error());
            }
            for (auto &&message : multiple_messages.value())
            {
                std::visit(detail::overloaded{
                               [](std::monostate) { /* todo make error */ },
                               [&do_receive](TStopMsg /*stop_msg*/) { do_receive = false; },
                               [&messages](auto &&msg) { messages.emplace_back(std::forward<decltype(msg)>(msg)); },
                           },
                           std::move(message));
            }
        }
        co_return messages;
    }

  private:
    std::string hostname_;
    std::uint16_t port_;
    std::string password_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    tcp::socket socket_;

    std::string device_name_;
    std::optional<ApiVersion> api_version_;
};
} // namespace cppesphomeapi
