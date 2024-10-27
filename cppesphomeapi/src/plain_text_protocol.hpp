#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <google/protobuf/message.h>
#include "cppesphomeapi/result.hpp"

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
} // namespace cppesphomeapi
