#include "log_entry.hpp"

void to_json(nlohmann::json& j, const LogEntry& log_entry) {
    j = json{{"timestamp", log_entry.timestamp},
             {"type", static_cast<std::uint8_t>(log_entry.type)},
             {"prefix", log_entry.prefix},
             {"tag", log_entry.tag},
             {"message", log_entry.message}};
}

void from_json(const nlohmann::json& j, LogEntry& log_entry) {
    log_entry.timestamp = j.at("timestamp").get<std::uint32_t>();
    log_entry.type = static_cast<LogType>(j.at("type").get<std::uint8_t>());
    log_entry.prefix = j.at("prefix").get<std::string>();
    log_entry.tag = j.at("tag").get<std::string>();
    log_entry.message = j.at("message").get<std::string>();
}