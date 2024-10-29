#pragma once
#include <cstdint>
#include <span>
#include <google/protobuf/message.h>
#include "api_options.pb.h"
#include "cppesphomeapi/result.hpp"
#include "make_unexpected_result.hpp"
#include "message_wrapper.hpp"

#include <print>
namespace cppesphomeapi
{

struct PlainTextProtocol
{
    static Result<std::vector<std::uint8_t>> serialize(const ::google::protobuf::Message &message);

    template <typename TMsg>
    auto parse_and_invoke(google::protobuf::io::CodedInputStream *stream,
                          const std::uint32_t message_type,
                          auto &&message_handler) -> bool
    {
        if (TMsg::GetDescriptor()->options().GetExtension(proto::id) != message_type)
        {
            return false;
        }

        auto message = std::make_shared<TMsg>();
        const bool parsed = message->ParseFromCodedStream(stream);
        if (not parsed)
        {
            return false;
        }
        message_handler(MessageWrapper{std::move(message)});
        return true;
    }

    template <typename... TMsgs>
    auto decode_multiple(std::span<const std::uint8_t> received_data, auto &&message_handler) -> Result<void>
    {
        if (received_data.size() < 3)
        {
            return make_unexpected_result(ApiErrorCode::ParseError,
                                          "response does not contain enough bytes for the header");
        }

        google::protobuf::io::CodedInputStream stream{received_data.data(), static_cast<int>(received_data.size())};
        while (stream.BytesUntilLimit() > 0)
        {
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
            const auto accepted_msg =
                std::any_of(possible_ids.cbegin(), possible_ids.cend(), [message_type](auto &&desc_id) {
                    return desc_id->options().GetExtension(proto::id) == message_type;
                });
            if (not accepted_msg)
            {
                stream.Skip(static_cast<int>(message_size));
                std::println("Got not accepted message {}. Skipping {} bytes", message_type, message_size);
                continue;
            }

            if (stream.BytesUntilLimit() < message_size)
            {
                return make_unexpected_result(
                    ApiErrorCode::ParseError,
                    std::format(
                        "Received message size does not match the remaining bytes. Expected {} bytes got {} bytes.",
                        message_size,
                        stream.BytesUntilLimit()));
            }

            const bool parsed = (parse_and_invoke<TMsgs>(std::addressof(stream), message_type, message_handler) || ...);
            if (not parsed)
            {
                return make_unexpected_result(ApiErrorCode::ParseError,
                                              std::format("Could not parse any message from bytes."));
            }
        }
        return Result<void>{};
    }
};

} // namespace cppesphomeapi
