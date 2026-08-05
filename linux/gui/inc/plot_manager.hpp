#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <chrono>

#include "serial_port.hpp"
#include "recording.hpp"
#include "plot.hpp"

class Session;

enum class Panel { Signals, Sessions, Profiles };

class PlotManager {
  public:
    PlotManager();

    void draw_tiles(const Session& session);

    void add_plot();
    void update_recorders();

  private:
    struct PlotTile {
        std::uint32_t id;
        std::uint32_t plot_number;
        std::vector<std::uint8_t> signal_ids;

        bool follow = true;
        bool auto_y = true;
        bool paused = false;
        bool reset_view = true;

        float follow_seconds = 3600.0f;
        Recorder recorder;
    };

    static constexpr std::size_t max_plots = 6;
    static constexpr std::size_t max_signals_per_plot = 8;

    std::vector<PlotTile> plots_;
    std::uint32_t next_plot_id_ = 0;

    std::chrono::steady_clock::time_point last_recorders_flush_{std::chrono::steady_clock::now()};
    std::chrono::milliseconds flush_interval{1000};
};