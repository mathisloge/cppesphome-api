#ifndef CPPESPHOMEAPI_API_CLIENT_HPP
#define CPPESPHOMEAPI_API_CLIENT_HPP
#include <cstdint>
#include <memory>
#include <string>
#include <cppesphomeapi/cppesphomeapi_export.hpp>

namespace cppesphomeapi
{
class CPPESPHOMEAPI_EXPORT ApiClient
{
  public:
    ApiClient(std::string hostname, std::uint16_t port = 6053, std::string password = "");
    ~ApiClient();

    ApiClient(const ApiClient &) = delete;
    ApiClient(ApiClient &&) = delete;
    ApiClient &operator=(const ApiClient &) = delete;
    ApiClient &operator=(ApiClient &&) = delete;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace cppesphomeapi
#endif
