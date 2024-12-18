#ifndef CPPESPHOMEAPI_API_CLIENT_HPP
#define CPPESPHOMEAPI_API_CLIENT_HPP
#include <cstdint>
#include <memory>
#include <stop_token>
#include <string>
#include <cppesphomeapi/cppesphomeapi_export.hpp>
#include "api_version.hpp"
#include "async_result.hpp"
#include "commands.hpp"
#include "cppesphomeapi/log_entry.hpp"
#include "device_info.hpp"
#include "entity.hpp"
#include "state.hpp"

namespace cppesphomeapi
{
class ApiConnection;

using EntityInfoVariant = std::variant<EntityInfo, LightEntityInfo>;
using EntityInfoList = std::vector<EntityInfoVariant>;

using EntityStateVariant = std::variant<LightState>;

class CPPESPHOMEAPI_EXPORT ApiClient
{
  public:
    explicit ApiClient(const boost::asio::any_io_executor &executor,
                       std::stop_source &stop_source,
                       std::string hostname,
                       std::uint16_t port = 6053,
                       std::string password = "");
    ~ApiClient();

    [[nodiscard]] std::optional<ApiVersion> api_version() const;
    [[nodiscard]] const std::string &device_name() const;
    AsyncResult<void> async_connect();
    AsyncResult<void> async_disconnect();
    AsyncResult<DeviceInfo> async_device_info();
    AsyncResult<EntityInfoList> async_list_entities_services();
    AsyncResult<void> async_light_command(LightCommand light_command);
    AsyncResult<LogEntry> async_receive_log();
    AsyncResult<EntityStateVariant> async_receive_state();
    AsyncResult<void> subscribe_logs(EspHomeLogLevel log_level, bool config_dump);
    AsyncResult<void> subscribe_states();
    void close();

    ApiClient(const ApiClient &) = delete;
    ApiClient(ApiClient &&) = delete;
    ApiClient &operator=(const ApiClient &) = delete;
    ApiClient &operator=(ApiClient &&) = delete;

  private:
    std::unique_ptr<ApiConnection> connection_;
};
} // namespace cppesphomeapi
#endif
