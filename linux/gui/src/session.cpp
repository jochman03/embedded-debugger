#include "session.hpp"
#include <nlohmann/json.hpp>

void Session::add_sample(std::uint8_t signal_id, std::uint32_t timestamp_ms, double value,
                         double host_time_s) {
    PlotSeries& series = series_[signal_id];
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
    series.last_receive_time = host_time_s;
    series.has_sample = true;
    if (series.points.size() < max_samples_) {
        series.points.push_back(point);
        return;
    }

    series.points[series.offset] = point;
    series.offset = (series.offset + 1) % static_cast<int>(max_samples_);
}

std::uint32_t Session::id() const {
    return id_;
}

std::vector<LogEntry>& Session::logs() {
    return logs_;
}

const std::map<std::uint8_t, PlotSeries>& Session::getSeries() const {
    return series_;
}

void to_json(nlohmann::json& j, const Session& session) {
    std::map<std::uint8_t, PlotSeries> series;
    for (const auto& [id, s] : session.series_) {
        if (s.points.empty() && !s.has_sample) {
            continue;
        }
        series[id] = s;
    }
    j = json{{"id", session.id_}, {"logs", session.logs_}, {"series", series}};
}

void from_json(const nlohmann::json& j, Session& session) {
    session.id_ = j.at("id").get<std::uint32_t>();
    session.logs_ = j.at("logs").get<std::vector<LogEntry>>();
    if (j.contains("series")) {
        const json& s = j.at("series");
        if (s.is_array()) {
            session.series_ = s.get<std::map<std::uint8_t, PlotSeries>>();
        } else if (s.is_object()) {
            session.series_.clear();
            for (const auto& [key, val] : s.items()) {
                session.series_[static_cast<std::uint8_t>(std::stoul(key))] = val.get<PlotSeries>();
            }
        }
    } else {
        session.series_.clear();
    }
}