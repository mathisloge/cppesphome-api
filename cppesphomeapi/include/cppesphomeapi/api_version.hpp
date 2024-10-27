#ifndef CPPESPHOMEAPI_API_VERSION_HPP
#define CPPESPHOMEAPI_API_VERSION_HPP
#include <cstdint>

namespace cppesphomeapi
{
struct ApiVersion
{
    std::uint32_t major{};
    std::uint32_t minor{};
};
} // namespace cppesphomeapi

#endif
