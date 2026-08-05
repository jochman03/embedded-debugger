#pragma once

#include <string>
#include <cstdint>

enum class LogType { Info, Warning, Error, Reset, Unknown };

struct LogEntry {
    std::uint32_t timestamp = 0;
    LogType type = LogType::Unknown;
    std::string prefix;
    std::string tag;
    std::string message;
    bool active{false};
};

LogType prefix_to_log_type(const std::string& prefix);