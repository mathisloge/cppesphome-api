#include "plain_text_protocol.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "api_options.pb.h"
#include "make_unexpected_result.hpp"

namespace cppesphomeapi
{
Result<void> plain_text_decode(std::span<const std::uint8_t> received_data, ::google::protobuf::Message &message)
{
    if (received_data.size() < 3)
    {
        return make_unexpected_result(ApiErrorCode::ParseError,
                                      "response does not contain enough bytes for the header");
    }
    google::protobuf::io::CodedInputStream stream{received_data.data(), static_cast<int>(received_data.size())};
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

    const auto expected_message_id = message.GetDescriptor()->options().GetExtension(proto::id);
    if (expected_message_id != message_type)
    {
        return make_unexpected_result(
            ApiErrorCode::UnexpectedMessage,
            std::format("Expected {} but got message id {}", message.GetDescriptor()->name(), message_type));
    }
    if (stream.BytesUntilLimit() != message_size)
    {
        return make_unexpected_result(
            ApiErrorCode::ParseError,
            std::format("Received message size does not match the remaining bytes. Expected {} bytes got {} bytes.",
                        message_size,
                        stream.BytesUntilLimit()));
    }
    const bool parsed = message.ParseFromCodedStream(std::addressof(stream));
    if (not parsed)
    {
        return make_unexpected_result(
            ApiErrorCode::ParseError,
            std::format("Could not parse message \"{}\" from bytes.", message.GetDescriptor()->name()));
    }
    return {};
}

Result<std::vector<std::uint8_t>> plain_text_serialize(const ::google::protobuf::Message &message)
{
    constexpr std::uint8_t kPlainTextPreamble = 0x00;
    auto &&msg_options = message.GetDescriptor()->options();
    if (not msg_options.HasExtension(proto::id))
    {
        return make_unexpected_result(
            ApiErrorCode::SerializeError,
            std::format("message \"{}\" does not contain the id field", message.GetDescriptor()->name()));
    }

    constexpr auto kMaxHeaderLen = 5;
    std::vector<std::uint8_t> buffer;
    buffer.resize(message.ByteSizeLong() + kMaxHeaderLen);

    buffer[0] = kPlainTextPreamble;

    google::protobuf::io::ArrayOutputStream zstream{std::next(buffer.data(), 1), static_cast<int>(buffer.size() - 1)};
    google::protobuf::io::CodedOutputStream output_stream{std::addressof(zstream)};

    output_stream.WriteVarint32(message.ByteSizeLong());
    output_stream.WriteVarint32(msg_options.GetExtension(proto::id));

    if (output_stream.ByteCount() != (kMaxHeaderLen - 1))
    {
        buffer.resize(1 + message.ByteSizeLong() + output_stream.ByteCount());
    }

    const bool serialized = message.SerializeToCodedStream(std::addressof(output_stream));
    if (not serialized)
    {
        return make_unexpected_result(
            ApiErrorCode::SerializeError,
            std::format("could not serialize message \"{}\"", message.GetDescriptor()->name()));
    }
    return buffer;
}
} // namespace cppesphomeapi
