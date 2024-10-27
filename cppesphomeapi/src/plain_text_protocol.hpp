#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <google/protobuf/message.h>
#include "api_options.pb.h"
#include "cppesphomeapi/result.hpp"
#include "make_unexpected_result.hpp"

#include <print>
namespace cppesphomeapi
{
Result<void> plain_text_decode(std::span<const std::uint8_t> received_data, ::google::protobuf::Message &message);

template <typename TMsg>
Result<TMsg> plain_text_decode(std::span<const std::uint8_t> received_data)
{
    TMsg received_message;
    return plain_text_decode(received_data, received_message)
        .transform([received_message = std::move(received_message)]() { return received_message; });
}

Result<std::vector<std::uint8_t>> plain_text_serialize(const ::google::protobuf::Message &message);

// TODO: refactor with plain_text_decode

template <typename TMsg>
auto parse_msg(google::protobuf::io::CodedInputStream *stream, auto &result_variant, const std::uint32_t message_type)
    -> bool
{
    if (TMsg::GetDescriptor()->options().GetExtension(proto::id) != message_type)
    {
        return false;
    }

    TMsg message;
    const bool parsed = message.ParseFromCodedStream(stream);
    if (not parsed)
    {
        return false;
    }
    result_variant = std::move(message);
    std::println("got list message {}", TMsg::GetDescriptor()->name());
    return true;
}

template <typename... TMsgs>
auto plain_text_decode_multiple(std::span<const std::uint8_t> received_data)
    -> Result<std::vector<std::variant<std::monostate, TMsgs...>>>
{
    using ResultType = std::variant<std::monostate, TMsgs...>;
    if (received_data.size() < 3)
    {
        return make_unexpected_result(ApiErrorCode::ParseError,
                                      "response does not contain enough bytes for the header");
    }

    google::protobuf::io::CodedInputStream stream{received_data.data(), static_cast<int>(received_data.size())};
    std::vector<ResultType> messages;
    while (stream.BytesUntilLimit() > 0)
    {
        std::println("remaining bytes: {}", stream.BytesUntilLimit());
        std::uint32_t preamble{};
        if (not stream.ReadVarint32(&preamble) or preamble != 0x00)
        {
            return make_unexpected_result(ApiErrorCode::ParseError, "response does contain an invalid preamble");
        }

        std::uint32_t message_size{};
        if (not stream.ReadVarint32(&message_size))
        {
            return make_unexpected_result(ApiErrorCode::ParseError, "got no message size");
        }

        std::uint32_t message_type{};
        if (not stream.ReadVarint32(&message_type))
        {
            return make_unexpected_result(ApiErrorCode::ParseError, "got no message type");
        }

        const std::array<const google::protobuf::Descriptor *, sizeof...(TMsgs)> possible_ids{
            TMsgs::GetDescriptor()...};
        const auto valid_msg = std::any_of(possible_ids.cbegin(), possible_ids.cend(), [message_type](auto &&desc_id) {
            return desc_id->options().GetExtension(proto::id) == message_type;
        });
        if (not valid_msg)
        {
            return make_unexpected_result(ApiErrorCode::UnexpectedMessage,
                                          std::format("got unexpected message id {}", message_type));
        }

        if (stream.BytesUntilLimit() < message_size)
        {
            return make_unexpected_result(
                ApiErrorCode::ParseError,
                std::format("Received message size does not match the remaining bytes. Expected {} bytes got {} bytes.",
                            message_size,
                            stream.BytesUntilLimit()));
        }

        ResultType result;
        const bool parsed = (parse_msg<TMsgs>(std::addressof(stream), result, message_type) || ...);
        if (not parsed)
        {
            return make_unexpected_result(ApiErrorCode::ParseError,
                                          std::format("Could not parse any message from bytes."));
        }
        messages.emplace_back(std::move(result));
    }
    return messages;
}
} // namespace cppesphomeapi
