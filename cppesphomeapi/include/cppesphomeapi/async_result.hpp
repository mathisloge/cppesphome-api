#ifndef CPPESPHOMEAPI_ASYNC_RESULT_HPP
#define CPPESPHOMEAPI_ASYNC_RESULT_HPP
#include "detail/awaitable.hpp"
#include "result.hpp"

namespace cppesphomeapi
{
template <typename T>
using AsyncResult = awaitable<Result<T>>;
}
#endif
