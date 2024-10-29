#pragma once
#include <string>

namespace cppesphomeapi
{
enum class EspHomeLogLevel
{
    None = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Config = 4,
    Debug = 5,
    Verbose = 6,
    VeryVerbose = 7,
};

struct LogEntry
{
    EspHomeLogLevel log_level;
    std::string message;
};
} // namespace cppesphomeapi
