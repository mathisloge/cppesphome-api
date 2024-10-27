#ifndef CPPESPHOMEAPI_API_CLIENT_HPP
#define CPPESPHOMEAPI_API_CLIENT_HPP
#include <cstdint>
#include <memory>
#include <string>
#include <cppesphomeapi/cppesphomeapi_export.hpp>
#include "async_result.hpp"

namespace cppesphomeapi
{
class ApiConnection;

class CPPESPHOMEAPI_EXPORT ApiClient
{
  public:
    explicit ApiClient(const boost::asio::any_io_executor &executor, std::string hostname, std::uint16_t port = 6053, std::string password = "");
    ~ApiClient();

    AsyncResult<void> connect();

    ApiClient(const ApiClient &) = delete;
    ApiClient(ApiClient &&) = delete;
    ApiClient &operator=(const ApiClient &) = delete;
    ApiClient &operator=(ApiClient &&) = delete;

  private:
    std::unique_ptr<ApiConnection> connection_;
};
} // namespace cppesphomeapi
#endif
