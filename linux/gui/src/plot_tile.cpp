#include <algorithm>
#include <string>

#include "plot_tile.hpp"
#include "session.hpp"
#include "imgui.h"
#include "implot.h"

bool PlotTile::draw(const Session& session,
                    const std::map<std::uint8_t, std::string>& signal_registry, bool removable,
                    std::size_t max_signals) {
    bool remove_plot = false;
    int signal_to_remove = -1;
    const auto& series_map = session.getSeries();

    ImGui::PushID(static_cast<int>(id));

    ImGui::BeginChild("PlotTile", ImVec2(-1.0f, 320.0f), true);

    ImGui::Text("Plot %u", plot_number);
    ImGui::SameLine();
    ImGui::Checkbox("Follow", &follow);

    ImGui::SameLine();
    ImGui::Checkbox("Auto Y", &auto_y);

    ImGui::SameLine();
    ImGui::Checkbox("Pause", &paused);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f);
    ImGui::DragFloat("Window [s]", &follow_seconds, 0.1f, 0.1f, 3600.0f, "%.1f");

    ImGui::SameLine();
    if (ImGui::SmallButton("Reset")) {
        follow = true;
        auto_y = true;
        paused = false;
        reset_view = true;
    }
    ImGui::SameLine();
    bool recording = recorder.is_recording();
    if (recording) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
    if (ImGui::SmallButton("Record")) {
        if (recording) {
            recorder.stop_recording();
        } else {
            recorder.start_recording(plot_number);
        }
    }

    if (recording) {
        ImGui::PopStyleColor();
    }
    if (removable) {
        ImGui::SameLine();
        float button_pos_x =
            ImGui::GetContentRegionAvail().x - 20.0f - ImGui::CalcTextSize("Remove").x;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_pos_x);
        if (ImGui::SmallButton("Remove")) {
            remove_plot = true;
        }
    }

    const ImVec2 plot_size = ImGui::GetContentRegionAvail();

    double latest_time = 0.0;
    bool has_data = false;

    for (const std::uint8_t signal_id : signal_ids) {
        const auto series_it = series_map.find(signal_id);

        if (series_it == series_map.end()) {
            continue;
        }

        const PlotSeries& series = series_it->second;

        if (series.points.empty()) {
            continue;
        }

        const std::size_t newest_index =
            (static_cast<std::size_t>(series.offset) + series.points.size() - 1) %
            series.points.size();

        latest_time = std::max(latest_time, series.points[newest_index].timestamp);

        has_data = true;
    }

    if (reset_view) {
        ImPlot::SetNextAxesToFit();
        reset_view = false;
    }
    if (auto_y) {
        ImPlot::SetNextAxisToFit(ImAxis_Y1);
    }

    if (plot_size.x > 0.0f && plot_size.y > 0.0f && ImPlot::BeginPlot("##Plot", plot_size)) {
        const ImPlotAxisFlags x_flags = ImPlotAxisFlags_NoHighlight;

        ImPlotAxisFlags y_flags = ImPlotAxisFlags_NoHighlight;

        if (auto_y) {
            y_flags |= ImPlotAxisFlags_RangeFit;
        }
        ImPlot::SetupAxes("Time [s]", "Value", x_flags, y_flags);

        if (!paused && has_data && follow) {
            const double window = static_cast<double>(follow_seconds);

            ImPlot::SetupAxisLimits(ImAxis_X1, latest_time - window, latest_time,
                                    ImPlotCond_Always);
        }

        for (const std::uint8_t signal_id : signal_ids) {
            const auto series_it = series_map.find(signal_id);

            if (series_it == series_map.end()) {
                continue;
            }

            const PlotSeries& series = series_it->second;

            if (series.points.empty()) {
                continue;
            }

            std::string name;
            const auto name_it = signal_registry.find(signal_id);
            if (name_it != signal_registry.end()) {
                name = name_it->second;
            } else {
                name = "signal_" + std::to_string(signal_id);
            }

            const std::string label = name + "##signal_" + std::to_string(signal_id);

            ImPlotSpec spec;
            spec.Offset = series.offset;
            spec.Stride = static_cast<int>(sizeof(PlotPoint));

            ImPlot::PlotLine(label.c_str(), &series.points[0].timestamp, &series.points[0].value,
                             static_cast<int>(series.points.size()), spec);
            if (ImPlot::BeginLegendPopup(label.c_str())) {
                ImGui::TextUnformatted(name.c_str());

                ImGui::Separator();

                if (ImGui::MenuItem("Remove from plot")) {
                    signal_to_remove = static_cast<int>(signal_id);
                }
                ImPlot::EndLegendPopup();
            }
        }
        ImPlot::EndPlot();
        if (signal_to_remove >= 0) {
            const auto signal_it = std::find(signal_ids.begin(), signal_ids.end(),
                                             static_cast<std::uint8_t>(signal_to_remove));
            if (signal_it != signal_ids.end()) {
                signal_ids.erase(signal_it);
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLOT_SIGNAL_ID");

        if (payload != nullptr && payload->DataSize == sizeof(std::uint8_t)) {
            const auto signal_id = *static_cast<const std::uint8_t*>(payload->Data);

            const auto existing = std::find(signal_ids.begin(), signal_ids.end(), signal_id);

            if (existing == signal_ids.end() && signal_ids.size() < max_signals) {
                signal_ids.push_back(signal_id);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopID();

    ImGui::Spacing();
    return remove_plot;
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
