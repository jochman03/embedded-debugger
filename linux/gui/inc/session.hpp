#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

#include "log_entry.hpp"
#include "plot.hpp"

class Session {
  public:
    Session(std::uint32_t id) {
        id_ = id;
    }
    Session() {
        id_ = 0;
    };

    void add_sample(std::uint8_t signal_id, std::uint32_t timestamp_ms, double value,
                    double host_time_s);
    std::uint32_t id() const;
    std::vector<LogEntry>& logs();
    const std::map<std::uint8_t, PlotSeries>& getSeries() const;

    friend void to_json(nlohmann::json& j, const Session& session);
    friend void from_json(const nlohmann::json& j, Session& session);

  private:
    static constexpr std::size_t max_samples_ = 10000;

    std::uint32_t id_;

    std::vector<LogEntry> logs_;
    std::map<std::uint8_t, PlotSeries> series_;
};