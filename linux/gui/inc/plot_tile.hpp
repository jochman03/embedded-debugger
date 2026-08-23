#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "recording.hpp"

class Session;

using json = nlohmann::json;

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

    bool draw(const Session& session, const std::map<std::uint8_t, std::string>& signal_registry,
              bool removable, std::size_t max_signals);

    friend void to_json(json& j, const PlotTile& plot_tile);
    friend void from_json(const json& j, PlotTile& plot_tile);
};
