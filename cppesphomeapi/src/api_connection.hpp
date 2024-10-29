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
#include "make_unexpected_result.hpp"
#include "message_wrapper.hpp"
#include "overloaded.hpp"
#include "tcp.hpp"

namespace cppesphomeapi
{
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
    AsyncResult<void> enable_logs(EspHomeLogLevel log_level, bool config_dump);
    AsyncResult<LogEntry> receive_log();

  private:
    AsyncResult<void> send_message(const google::protobuf::Message &message);

    template <boost::asio::completion_token_for<void(MessageWrapper)> CompletionToken>
    auto async_receive_message(CompletionToken &&token)
    {
        auto init = [this](boost::asio::completion_handler_for<void(MessageWrapper)> auto handler) {
            std::unique_lock l{handler_mtx_};
            handlers_.emplace_back(std::move(handler));
        };
        return boost::asio::async_initiate<CompletionToken, void(MessageWrapper)>(
            init, // First, pass the function object that launches the operation,
            token // then the completion token that will be transformed to a handler,
        );        // and, finally, any additional arguments to the function object.
    }

    template <typename TMsg>
    auto receive_message(auto &&completion_token) -> AsyncResult<std::shared_ptr<TMsg>>
    {
        while (true)
        {
            const auto received_message =
                co_await async_receive_message(std::forward<decltype(completion_token)>(completion_token));
            if (received_message.template holds_message<TMsg>())
            {
                co_return received_message.template as<TMsg>();
            }
        }
        co_return make_unexpected_result(ApiErrorCode::UnexpectedMessage, "could not receive any message");
    }

    template <typename TMsg>
    static constexpr bool handle_message_impl(const auto &visitor, const MessageWrapper &wrapper)
    {
        if (not wrapper.holds_message<TMsg>())
        {
            return false;
        }
        visitor(wrapper.as<TMsg>());
        return true;
    }
    template <typename... TMsgs>
    static constexpr void handle_messages(auto &&visitor, const MessageWrapper &wrapper)
    {
        (handle_message_impl<TMsgs>(visitor, wrapper) || ...);
    }

    template <typename TStopMsg, typename... TMsgs>
    auto receive_messages(auto &&comletion_token) -> AsyncResult<std::vector<std::variant<std::shared_ptr<TMsgs>...>>>
    {
        std::vector<std::variant<std::shared_ptr<TMsgs>...>> messages;
        messages.reserve(sizeof...(TMsgs)); // at least enough space to receive each message once.
        // todo: add stop source
        bool do_receive{true};
        while (do_receive)
        {
            const MessageWrapper message =
                co_await async_receive_message(std::forward<decltype(comletion_token)>(comletion_token));

            const bool accepted_msg = message.holds_message<TStopMsg>() || (message.holds_message<TMsgs>() || ...);
            if (not accepted_msg)
            {
                continue;
            }
            handle_messages<TStopMsg, TMsgs...>(
                detail::overloaded{
                    [&do_receive](std::shared_ptr<TStopMsg> /*stop_msg*/) { do_receive = false; },
                    [&messages](auto &&msg) { messages.emplace_back(std::forward<decltype(msg)>(msg)); },
                },
                message);
        }
        co_return messages;
    }

  private:
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
    std::vector<boost::asio::any_completion_handler<void(MessageWrapper)>> handlers_;
};
} // namespace cppesphomeapi
