#pragma once
#include "cppesphomeapi/result.hpp"

namespace cppesphomeapi
{
std::unexpected<ApiError> make_unexpected_result(ApiErrorCode code, std::string message);
}
