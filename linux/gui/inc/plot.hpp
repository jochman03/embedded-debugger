#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

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
