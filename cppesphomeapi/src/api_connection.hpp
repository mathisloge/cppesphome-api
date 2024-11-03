#pragma once
#include <cstdint>
#include <stop_token>
#include <string>
#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
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
#include "net.hpp"
#include "overloaded.hpp"

namespace cppesphomeapi
{
class ApiConnection
{
  public:
    explicit ApiConnection(std::string hostname,
                           std::uint16_t port,
                           std::string password,
                           std::stop_source &stop_source,
                           const boost::asio::any_io_executor &executor);

    AsyncResult<void> connect();
    AsyncResult<void> disconnect();
    AsyncResult<void> send_message_hello();
    AsyncResult<void> send_message_connect();
    AsyncResult<DeviceInfo> request_device_info();
    AsyncResult<EntityInfoList> request_entities_and_services();
    AsyncResult<void> light_command(LightCommand light_command);
    const std::optional<ApiVersion> &api_version() const;
    const std::string &device_name() const;
    AsyncResult<void> enable_logs(EspHomeLogLevel log_level, bool config_dump);
    AsyncResult<LogEntry> receive_log();
    AsyncResult<void> subscribe_states();
    AsyncResult<void> receive_state();

    void cancel();

  private:
    AsyncResult<void> send_message(const google::protobuf::Message &message);

    template <boost::asio::completion_token_for<void(MessageWrapper)> CompletionToken>
    auto async_receive_message(CompletionToken &&token)
    {
        auto init = [this](boost::asio::completion_handler_for<void(MessageWrapper)> auto handler) {
            std::unique_lock l{handler_mtx_};
            handlers_.emplace_back(std::move(handler));
        };
        return boost::asio::async_initiate<CompletionToken, void(MessageWrapper)>(init, token);
    }

    template <typename TMsg>
    auto receive_message(auto &&completion_token) -> AsyncResult<std::shared_ptr<TMsg>>
    {
        namespace aex = boost::asio::experimental;
        using aex::awaitable_operators::operator||;
        auto executor = co_await boost::asio::this_coro::executor;
        net::Timer timer{executor};
        timer.expires_after(std::chrono::milliseconds{250});
        while (true)
        {
            //! TODO: the added handler might get aborted. How to remove it from the vector holding the completion
            //! handlers?
            const auto received_message_or_error =
                co_await (async_receive_message(std::forward<decltype(completion_token)>(completion_token)) ||
                          timer.async_wait());
            if (std::holds_alternative<std::tuple<net::ErrorCode>>(received_message_or_error))
            {
                break;
            }
            auto &&received_message = std::get<cppesphomeapi::MessageWrapper>(received_message_or_error);
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
    boost::asio::awaitable<void> receive_loop();
    boost::asio::awaitable<void> heartbeat_loop();

  private:
    std::string hostname_;
    std::uint16_t port_;
    std::string password_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    net::Socket socket_;

    std::string device_name_;
    std::optional<ApiVersion> api_version_;

    mutable std::mutex handler_mtx_;
    std::vector<boost::asio::any_completion_handler<void(MessageWrapper)>> handlers_;
};
} // namespace cppesphomeapi
