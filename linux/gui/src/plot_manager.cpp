#include <algorithm>

#include "plot_manager.hpp"
#include "imgui.h"
#include "implot.h"
#include "protocol_command.hpp"
#include "session.hpp"
#include "application.hpp"

void PlotManager::draw_tiles(const Session& session) {
    ImGui::TextUnformatted("Plots");
    ImGui::Separator();

    auto series_ = session.getSeries();

    if (ImGui::Button("Add plot")) {
        add_plot();
    }

    ImGui::Separator();

    int plot_to_remove = -1;

    for (std::size_t index = 0; index < plots_.size(); index++) {
        PlotTile& plot = plots_[index];
        int signal_to_remove = -1;

        ImGui::PushID(static_cast<int>(plot.id));

        ImGui::BeginChild("PlotTile", ImVec2(-1.0f, 320.0f), true);

        ImGui::Text("Plot %zu", plot.plot_number);
        ImGui::SameLine();
        ImGui::Checkbox("Follow", &plot.follow);

        ImGui::SameLine();
        ImGui::Checkbox("Auto Y", &plot.auto_y);

        ImGui::SameLine();
        ImGui::Checkbox("Pause", &plot.paused);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f);
        ImGui::DragFloat("Window [s]", &plot.follow_seconds, 0.1f, 0.1f, 3600.0f, "%.1f");

        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) {
            plot.follow = true;
            plot.auto_y = true;
            plot.paused = false;
            plot.reset_view = true;
        }
        ImGui::SameLine();
        bool recording = plot.recorder.is_recording();
        if (recording) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        }
        if (ImGui::SmallButton("Record")) {
            if (recording) {
                plot.recorder.stop_recording();
            } else {
                plot.recorder.start_recording(plot.plot_number);
            }
        }

        if (recording) {
            ImGui::PopStyleColor();
        }
        if (plots_.size() > 1) {
            ImGui::SameLine();
            float button_pos_x =
                ImGui::GetContentRegionAvail().x - 20.0f - ImGui::CalcTextSize("Remove").x;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_pos_x);
            if (ImGui::SmallButton("Remove")) {
                plot_to_remove = static_cast<int>(index);
            }
        }

        const ImVec2 plot_size = ImGui::GetContentRegionAvail();

        double latest_time = 0.0;
        bool has_data = false;

        for (const std::uint8_t signal_id : plot.signal_ids) {
            const auto series_it = series_.find(signal_id);

            if (series_it == series_.end()) {
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

        if (plot.reset_view) {
            ImPlot::SetNextAxesToFit();
            plot.reset_view = false;
        } else if (!plot.paused && has_data) {
            if (plot.follow) {
                const double window = static_cast<double>(plot.follow_seconds);

                ImPlot::SetNextAxisLimits(ImAxis_X1, latest_time - window, latest_time,
                                          ImPlotCond_Always);
            }
        }
        if (plot.auto_y) {
            ImPlot::SetNextAxisToFit(ImAxis_Y1);
        }

        if (plot_size.x > 0.0f && plot_size.y > 0.0f && ImPlot::BeginPlot("##Plot", plot_size)) {
            const ImPlotAxisFlags x_flags = ImPlotAxisFlags_NoHighlight;

            ImPlotAxisFlags y_flags = ImPlotAxisFlags_NoHighlight;

            if (plot.auto_y) {
                y_flags |= ImPlotAxisFlags_RangeFit;
            }
            ImPlot::SetupAxes("Time [s]", "Value", x_flags, y_flags);

            for (const std::uint8_t signal_id : plot.signal_ids) {
                const auto series_it = series_.find(signal_id);

                if (series_it == series_.end()) {
                    continue;
                }

                const PlotSeries& series = series_it->second;

                if (series.points.empty()) {
                    continue;
                }

                const std::string label = series.name + "##signal_" + std::to_string(signal_id);

                ImPlotSpec spec;
                spec.Offset = series.offset;
                spec.Stride = static_cast<int>(sizeof(PlotPoint));

                ImPlot::PlotLine(label.c_str(), &series.points[0].timestamp,
                                 &series.points[0].value, static_cast<int>(series.points.size()),
                                 spec);
                if (ImPlot::BeginLegendPopup(label.c_str())) {
                    ImGui::TextUnformatted(series.name.c_str());

                    ImGui::Separator();

                    if (ImGui::MenuItem("Remove from plot")) {
                        signal_to_remove = static_cast<int>(signal_id);
                    }
                    ImPlot::EndLegendPopup();
                }
            }
            ImPlot::EndPlot();
            if (signal_to_remove >= 0) {
                const auto signal_it = std::find(plot.signal_ids.begin(), plot.signal_ids.end(),
                                                 static_cast<std::uint8_t>(signal_to_remove));
                if (signal_it != plot.signal_ids.end()) {
                    plot.signal_ids.erase(signal_it);
                }
            }
        }
        ImGui::EndChild();

        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLOT_SIGNAL_ID");

            if (payload != nullptr && payload->DataSize == sizeof(std::uint8_t)) {
                const auto signal_id = *static_cast<const std::uint8_t*>(payload->Data);

                const auto existing =
                    std::find(plot.signal_ids.begin(), plot.signal_ids.end(), signal_id);

                if (existing == plot.signal_ids.end() &&
                    plot.signal_ids.size() < max_signals_per_plot) {
                    plot.signal_ids.push_back(signal_id);
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();

        ImGui::Spacing();
    }
    if (plot_to_remove >= 0) {
        plots_.erase(plots_.begin() + plot_to_remove);
    }
}

PlotManager::PlotManager() {
    add_plot();
}

void PlotManager::add_plot() {
    if (plots_.size() >= max_plots) {
        return;
    }

    std::uint32_t free_number = 1;

    for (; free_number <= max_plots; ++free_number) {
        bool number_used = false;

        for (const PlotTile& plot : plots_) {
            if (plot.plot_number == free_number) {
                number_used = true;
                break;
            }
        }

        if (!number_used) {
            break;
        }
    }

    auto insert_position = plots_.begin();

    while (insert_position != plots_.end() && insert_position->plot_number < free_number) {
        ++insert_position;
    }

    auto inserted_plot = plots_.emplace(insert_position);

    inserted_plot->plot_number = free_number;
    inserted_plot->id = next_plot_id_++;
}

void PlotManager::update_recorders() {
    auto now = std::chrono::steady_clock::now();
    if (now - last_recorders_flush_ >= flush_interval) {
        for (auto& plot : plots_) {
            if (!plot.recorder.is_recording()) {
                continue;
            }
            if (plot.recorder.has_data_pending()) {
                plot.recorder.flush();
            }
        }
        last_recorders_flush_ = now;
    }
}
