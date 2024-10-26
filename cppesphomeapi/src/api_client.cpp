#include "cppesphomeapi/api_client.hpp"
namespace cppesphomeapi
{
class ApiClient::Impl
{};

ApiClient::ApiClient(std::string hostname, std::uint16_t port, std::string password)
    : impl_{std::make_unique<Impl>()}
{}

ApiClient::~ApiClient() = default;
} // namespace cppesphomeapi
