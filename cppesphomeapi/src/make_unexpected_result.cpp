#include "make_unexpected_result.hpp"
namespace cppesphomeapi
{
std::unexpected<ApiError> make_unexpected_result(ApiErrorCode code, std::string message)
{
    return std::unexpected(ApiError{.code = code, .message = std::move(message)});
};
} // namespace cppesphomeapi
