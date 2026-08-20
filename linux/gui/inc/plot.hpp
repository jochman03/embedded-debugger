#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "recording.hpp"

using json = nlohmann::json;

struct PlotPoint {
    double timestamp;
    double value;

    friend void to_json(json& j, const PlotPoint& plot_point);
    friend void from_json(const json& j, PlotPoint& plot_point);
};

struct PlotSeries {
    std::string name;
    std::vector<PlotPoint> points;
    double last_value = 0.0;
    double last_timestamp = 0.0;
    double frequency_hz = 0.0;
    int sample_count = 0;
    int offset = 0;
    double last_receive_time = 0.0;
    bool has_sample = false;

    friend void to_json(json& j, const PlotSeries& plot_series);
    friend void from_json(const json& j, PlotSeries& plot_series);
};

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

    friend void to_json(json& j, const PlotTile& plot_tile);
    friend void from_json(const json& j, PlotTile& plot_tile);
};
