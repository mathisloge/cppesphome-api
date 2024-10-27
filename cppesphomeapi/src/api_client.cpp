#include "cppesphomeapi/api_client.hpp"
#include "api_connection.hpp"
namespace cppesphomeapi
{
ApiClient::ApiClient(const boost::asio::any_io_executor &executor,
                     std::string hostname,
                     std::uint16_t port,
                     std::string password)
    : connection_{std::make_unique<ApiConnection>(std::move(hostname), port, std::move(password), executor)}
{}

ApiClient::~ApiClient() = default;

AsyncResult<void> ApiClient::connect()
{
    co_return co_await connection_->connect();
}
} // namespace cppesphomeapi
