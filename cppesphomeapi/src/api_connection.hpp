#pragma once
#include <cstdint>
#include <string>
#include <boost/asio/executor.hpp>
#include <boost/asio/strand.hpp>
#include <google/protobuf/message.h>
#include "cppesphomeapi/async_result.hpp"
#include "make_unexpected_result.hpp"
#include "plain_text_protocol.hpp"
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
    AsyncResult<void> send_message_hello();

  private:
    AsyncResult<void> send_message(const google::protobuf::Message &message);

    template <typename TMsg>
    AsyncResult<TMsg> receive_message()
    {
        namespace asio = boost::asio;
        std::array<std::uint8_t, 512> buffer{};
        const auto received_bytes = co_await socket_.async_receive(asio::buffer(buffer));
        if (received_bytes < 3)
        {
            co_return make_unexpected_result(ApiErrorCode::ParseError,
                                             "response does not contain enough bytes for the header");
        }
        co_return plain_text_decode<TMsg>(std::span{buffer.begin(), received_bytes});
    }

  private:
    std::string hostname_;
    std::uint16_t port_;
    std::string password_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    tcp::socket socket_;
};
} // namespace cppesphomeapi
