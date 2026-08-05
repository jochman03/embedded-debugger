#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct PlotPoint {
    double timestamp;
    double value;
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
};