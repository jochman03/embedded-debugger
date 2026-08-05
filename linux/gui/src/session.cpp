#include "session.hpp"
#include "plot_manager.hpp"
#include "imgui.h"

void Session::add_sample(std::uint8_t signal_id, const std::string& name,
                         std::uint32_t timestamp_ms, double value) {
    PlotSeries& series = series_[signal_id];
    if (series.name.empty()) {
        series.name = name;
    }
    if (series.points.capacity() == 0) {
        series.points.reserve(max_samples_);
    }
    const PlotPoint point{static_cast<double>(timestamp_ms) / 1000.0, value};

    const double delta_timestamp = point.timestamp - series.last_timestamp;
    if (delta_timestamp > 0) {
        double current_frequency = 1.0 / delta_timestamp;
        if (series.frequency_hz == 0.0) {
            series.frequency_hz = current_frequency;
        } else {
            series.frequency_hz = 0.9 * series.frequency_hz + 0.1 * current_frequency;
        }
    }

    series.sample_count++;
    series.last_value = value;
    series.last_timestamp = static_cast<double>(timestamp_ms) / 1000.0;
    series.last_receive_time = ImGui::GetTime();
    series.has_sample = true;
    if (series.points.size() < max_samples_) {
        series.points.push_back(point);
        return;
    }

    series.points[series.offset] = point;
    series.offset = (series.offset + 1) % static_cast<int>(max_samples_);
}

void Session::register_signal(std::uint8_t signal_id, const std::string& name) {
    PlotSeries& series = series_[signal_id];
    series.name = name;
}

std::uint32_t Session::id() const {
    return id_;
}

const std::map<std::uint8_t, PlotSeries>& Session::getSeries() const {
    return series_;
}
