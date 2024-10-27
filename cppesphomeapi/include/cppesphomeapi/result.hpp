#ifndef CPPESPHOMEAPI_RESULT_HPP
#define CPPESPHOMEAPI_RESULT_HPP
#include <expected>
#include <string>

namespace cppesphomeapi
{

enum ApiErrorCode
{
    SerializeError,
    ParseError,
    UnexpectedMessage,
    SendError
};

struct ApiError
{
    ApiErrorCode code;
    std::string message;
};

template <typename T>
using Result = std::expected<T, ApiError>;
} // namespace cppesphomeapi
#endif
