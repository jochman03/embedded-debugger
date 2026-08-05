#pragma once

#include <cstdint>
#include "log_entry.hpp"
#include "plot_manager.hpp"
#include "plot.hpp"

class Session {
  public:
    Session(std::uint32_t id) {
        id_ = id;
    }
    void add_sample(std::uint8_t signal_id, const std::string& name, std::uint32_t timestamp_ms,
                    double value);
    void register_signal(std::uint8_t signal_id, const std::string& name);
    std::uint32_t id() const;
    const std::map<std::uint8_t, PlotSeries>& getSeries() const;

  private:
    static constexpr std::size_t max_samples_ = 10000;

    std::uint32_t id_;

    std::vector<LogEntry> logs_;
    std::map<std::uint8_t, PlotSeries> series_;
};