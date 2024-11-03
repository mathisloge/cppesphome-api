/**
 * FROM https://github.com/DanielaE/CppInAction
 * with adaptions for this project
 */

#pragma once
#include <expected>
#include <tuple>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace cppesphomeapi::net
{
namespace asio = boost::asio;

using ErrorCode = boost::system::error_code;

template <typename... Ts>
using AsioResult = std::tuple<ErrorCode, Ts...>;
using use_await = asio::as_tuple_t<asio::use_awaitable_t<>>;
using Socket = use_await::as_default_on_t<asio::ip::tcp::socket>;
using Acceptor = use_await::as_default_on_t<asio::ip::tcp::acceptor>;
using Timer = use_await::as_default_on_t<asio::steady_timer>;
using Endpoint = asio::ip::tcp::endpoint;
using Endpoints = std::span<const Endpoint>;
enum Port : uint16_t
{
};

template <typename T>
using Expected = std::expected<T, ErrorCode>;
using ExpectSize = Expected<std::size_t>;
using ExpectSocket = Expected<Socket>;

template <std::size_t N>
using SendBuffers = std::array<asio::const_buffer, N>;
using ConstBuffers = std::span<asio::const_buffer>;
using ByteSpan = std::span<std::byte>;
using ConstByteSpan = std::span<const std::byte>;

auto sendTo(Socket &socket, Timer &timer, ConstByteSpan data) -> asio::awaitable<ExpectSize>;
auto receiveFrom(Socket &socket, Timer &timer, ByteSpan byte_buffer) -> asio::awaitable<ExpectSize>;
auto connectTo(Endpoints endpoints, Timer &timer) -> asio::awaitable<ExpectSocket>;
auto expired(Timer &timer) noexcept -> asio::awaitable<bool>;

void close(Socket &socket) noexcept;
auto resolveHostEndpoints(std::string_view HostName, Port port, Timer &timer)
    -> asio::awaitable<Expected<std::vector<Endpoint>>>;

namespace detail
{
// transform the 'variant' return type from asio operator|| into an 'expected'
// as simply as possible to scare away no one. No TMP required here!

template <typename R, typename... Ts>
constexpr auto map(AsioResult<Ts...> &&tuple) -> Expected<R>
{
    const auto &error = std::get<ErrorCode>(tuple);
    if constexpr (sizeof...(Ts) == 0)
    {
        return std::unexpected{error};
    }
    if (error)
    {
        return std::unexpected{error};
    }
    if constexpr (sizeof...(Ts) > 0)
    {
        return std::get<R>(std::move(tuple));
    }
    return R{};
}

template <typename... Ts, typename... Us>
constexpr auto flatten(std::variant<AsioResult<Ts...>, AsioResult<Us...>> &&variant)
{
    using ReturnType = std::type_identity<Ts..., Us...>::type;
    return std::visit([](auto &&tuple) { return map<ReturnType>(std::forward<decltype(tuple)>(tuple)); },
                      std::move(variant));
}

template <typename Out, typename In>
auto replace(net::Expected<In> &&Input, Out &&Replacement) -> net::Expected<Out>
{
    return std::move(Input).transform([&](In &&) { return std::forward<Out>(Replacement); });
}
} // namespace detail
} // namespace cppesphomeapi::net
