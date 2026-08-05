#include "log_entry.hpp"

LogType prefix_to_log_type(const std::string& prefix) {
    if (prefix == "[INFO]") {
        return LogType::Info;
    }

    if (prefix == "[WARNING]") {
        return LogType::Warning;
    }

    if (prefix == "[ERROR]") {
        return LogType::Error;
    }
    if (prefix == "[RESET]") {
        return LogType::Reset;
    }
    return LogType::Unknown;
}