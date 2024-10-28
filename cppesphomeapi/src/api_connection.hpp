#pragma once
#include <cstdint>
#include <string>
#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <google/protobuf/message.h>
#include "api.pb.h"
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

    template <boost::asio::completion_token_for<void(std::shared_ptr<google::protobuf::Message>)> CompletionToken>
    auto async_receive_message(CompletionToken &&token)
    {
        auto init =
            [this](boost::asio::completion_handler_for<void(std::shared_ptr<google::protobuf::Message>)> auto handler) {
                std::unique_lock l{handler_mtx_};
                handlers_.emplace_back(std::move(handler));
            };
        return boost::asio::async_initiate<CompletionToken, void(std::shared_ptr<google::protobuf::Message>)>(
            init, // First, pass the function object that launches the operation,
            token // then the completion token that will be transformed to a handler,
        );        // and, finally, any additional arguments to the function object.
    }

    template <typename TMsg>
    auto receive_message() -> AsyncResult<std::shared_ptr<TMsg>>
    {
        while (true)
        {
            const auto received_message = co_await async_receive_message(boost::asio::use_awaitable);
            auto msg = std::dynamic_pointer_cast<TMsg>(received_message);
            if (msg != nullptr)
            {
                co_return msg;
            }
        }
        co_return make_unexpected_result(ApiErrorCode::UnexpectedMessage, "could not receive any message");
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
            const std::shared_ptr<google::protobuf::Message> message =
                co_await async_receive_message(boost::asio::use_awaitable);
            std::println("received a message", message->GetTypeName());
            // std::visit(detail::overloaded{
            //                [](std::monostate) { /* todo make error */ },
            //                [&do_receive](TStopMsg /*stop_msg*/) { do_receive = false; },
            //                [&messages](auto &&msg) { messages.emplace_back(std::forward<decltype(msg)>(msg)); },
            //            },
            //            std::move(message));
        }
        co_return messages;
    }

  private:
    boost::asio::awaitable<void> subscribe_logs();
    boost::asio::awaitable<void> async_receive();

  private:
    std::string hostname_;
    std::uint16_t port_;
    std::string password_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    tcp::socket socket_;

    std::string device_name_;
    std::optional<ApiVersion> api_version_;

    mutable std::mutex handler_mtx_;
    std::vector<boost::asio::any_completion_handler<void(std::shared_ptr<google::protobuf::Message> message)>>
        handlers_;
};
} // namespace cppesphomeapi
