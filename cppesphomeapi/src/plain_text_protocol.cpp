#include "plain_text_protocol.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "api_options.pb.h"
#include "make_unexpected_result.hpp"

namespace cppesphomeapi
{
Result<std::vector<std::byte>> PlainTextProtocol::serialize(const ::google::protobuf::Message &message)
{
    constexpr std::byte kPlainTextPreamble = std::byte{0x00};
    auto &&msg_options = message.GetDescriptor()->options();
    if (not msg_options.HasExtension(proto::id))
    {
        return make_unexpected_result(
            ApiErrorCode::SerializeError,
            std::format("message \"{}\" does not contain the id field", message.GetDescriptor()->name()));
    }

    constexpr auto kMaxHeaderLen = 5;
    std::vector<std::byte> buffer;
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
