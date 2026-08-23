#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <chrono>
#include <map>
#include <nlohmann/json.hpp>

#include "plot_tile.hpp"

class Session;

using json = nlohmann::json;

class PlotManager {
  public:
    PlotManager();

    void draw_tiles(const Session& session,
                    const std::map<std::uint8_t, std::string>& signal_registry);
    void add_plot();
    void update_recorders();

    friend void to_json(json& j, const PlotManager& plot_manager);
    friend void from_json(const json& j, PlotManager& plot_manager);

  private:
    static constexpr std::size_t max_plots = 6;
    static constexpr std::size_t max_signals_per_plot = 8;
    static constexpr std::chrono::milliseconds flush_interval{1000};

    std::vector<PlotTile> plots_;
    std::uint32_t next_plot_id_ = 0;

    std::chrono::steady_clock::time_point last_recorders_flush_{std::chrono::steady_clock::now()};
};