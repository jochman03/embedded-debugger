#pragma once

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "log_type.hpp"

using json = nlohmann::json;

struct LogEntry {
    std::uint32_t timestamp = 0;
    LogType type = LogType::Unknown;
    std::string prefix;
    std::string tag;
    std::string message;
    bool active{false};

    friend void to_json(nlohmann::json& j, const LogEntry& log_entry);
    friend void from_json(const nlohmann::json& j, LogEntry& log_entry);
};