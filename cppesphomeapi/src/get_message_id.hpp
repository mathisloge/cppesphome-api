#pragma once
#include <cstdint>
#include "api_options.pb.h"

namespace cppesphomeapi::detail
{
template <typename TMsg>
std::uint32_t get_message_id()
{
    return std::remove_cvref_t<TMsg>::GetDescriptor()->options().GetExtension(proto::id);
}
} // namespace cppesphomeapi::detail
