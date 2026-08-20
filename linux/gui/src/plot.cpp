#include "plot.hpp"

void to_json(json& j, const PlotPoint& plot_point) {
    j = json{{"timestamp", plot_point.timestamp}, {"value", plot_point.value}};
}

void from_json(const json& j, PlotPoint& plot_point) {
    plot_point.timestamp = j.at("timestamp").get<double>();
    plot_point.value = j.at("value").get<double>();
}

void to_json(json& j, const PlotSeries& plot_series) {
    j = json{{"name", plot_series.name},
             {"points", plot_series.points},
             {"last_value", plot_series.last_value},
             {"last_timestamp", plot_series.last_timestamp},
             {"frequency_hz", plot_series.frequency_hz},
             {"sample_count", plot_series.sample_count},
             {"offset", plot_series.offset},
             {"last_receive_time", plot_series.last_receive_time},
             {"has_sample", plot_series.has_sample}};
}

void from_json(const json& j, PlotSeries& plot_series) {
    plot_series.name = j.at("name").get<std::string>();
    plot_series.points = j.at("points").get<std::vector<PlotPoint>>();
    plot_series.last_value = j.at("last_value").get<double>();
    plot_series.last_timestamp = j.at("last_timestamp").get<double>();
    plot_series.frequency_hz = j.at("frequency_hz").get<double>();
    plot_series.sample_count = j.at("sample_count").get<int>();
    plot_series.offset = j.at("offset").get<int>();
    plot_series.last_receive_time = j.at("last_receive_time").get<double>();
    plot_series.has_sample = j.at("has_sample").get<bool>();
}

void to_json(json& j, const PlotTile& plot_tile) {
    j = json{{"id", plot_tile.id},
             {"plot_number", plot_tile.plot_number},
             {"signal_ids", plot_tile.signal_ids},
             {"follow", plot_tile.follow},
             {"auto_y", plot_tile.auto_y},
             {"paused", plot_tile.paused},
             {"follow_seconds", plot_tile.follow_seconds}};
}

void from_json(const json& j, PlotTile& plot_tile) {
    plot_tile.id = j.at("id").get<std::uint32_t>();
    plot_tile.plot_number = j.at("plot_number").get<std::uint32_t>();
    plot_tile.signal_ids = j.at("signal_ids").get<std::vector<std::uint8_t>>();
    plot_tile.follow = j.at("follow").get<bool>();
    plot_tile.auto_y = j.at("auto_y").get<bool>();
    plot_tile.paused = j.at("paused").get<bool>();
    plot_tile.follow_seconds = j.at("follow_seconds").get<float>();
}
