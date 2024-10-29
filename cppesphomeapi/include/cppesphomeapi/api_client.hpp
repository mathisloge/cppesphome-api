#ifndef CPPESPHOMEAPI_API_CLIENT_HPP
#define CPPESPHOMEAPI_API_CLIENT_HPP
#include <cstdint>
#include <memory>
#include <string>
#include <cppesphomeapi/cppesphomeapi_export.hpp>
#include "api_version.hpp"
#include "async_result.hpp"
#include "commands.hpp"
#include "cppesphomeapi/log_entry.hpp"
#include "device_info.hpp"
#include "entity.hpp"

namespace cppesphomeapi
{
class ApiConnection;

class CPPESPHOMEAPI_EXPORT ApiClient
{
  public:
    explicit ApiClient(const boost::asio::any_io_executor &executor,
                       std::string hostname,
                       std::uint16_t port = 6053,
                       std::string password = "");
    ~ApiClient();

    [[nodiscard]] std::optional<ApiVersion> api_version() const;
    [[nodiscard]] const std::string &device_name() const;
    AsyncResult<void> async_connect();
    AsyncResult<void> async_disconnect();
    AsyncResult<DeviceInfo> async_device_info();
    AsyncResult<std::vector<EntityInfo>> async_list_entities_services();
    AsyncResult<void> async_light_command(LightCommand light_command);
    AsyncResult<void> enable_logs(EspHomeLogLevel log_level, bool config_dump);
    AsyncResult<LogEntry> receive_log();

    ApiClient(const ApiClient &) = delete;
    ApiClient(ApiClient &&) = delete;
    ApiClient &operator=(const ApiClient &) = delete;
    ApiClient &operator=(ApiClient &&) = delete;

  private:
    std::unique_ptr<ApiConnection> connection_;
};
} // namespace cppesphomeapi
#endif
