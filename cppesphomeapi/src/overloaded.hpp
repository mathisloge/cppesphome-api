#pragma once

namespace cppesphomeapi::detail
{
template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
} // namespace cppesphomeapi::detail
