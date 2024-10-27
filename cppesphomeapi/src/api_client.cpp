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

AsyncResult<void> ApiClient::async_connect()
{
    co_return co_await connection_->connect();
}

AsyncResult<void> ApiClient::async_disconnect()
{
    co_return co_await connection_->disconnect();
}

AsyncResult<DeviceInfo> ApiClient::async_device_info()
{
    co_return co_await connection_->request_device_info();
}

AsyncResult<std::vector<EntityInfo>> ApiClient::async_list_entities_services()
{
    co_return co_await connection_->request_entities_and_services();
}

AsyncResult<void> ApiClient::async_light_command(LightCommand light_command)
{
    co_return co_await connection_->light_command(std::move(light_command));
}

std::optional<ApiVersion> ApiClient::api_version() const
{
    return connection_->api_version();
}

const std::string &ApiClient::device_name() const
{
    return connection_->device_name();
}

} // namespace cppesphomeapi
