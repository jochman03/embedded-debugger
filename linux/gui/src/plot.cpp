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