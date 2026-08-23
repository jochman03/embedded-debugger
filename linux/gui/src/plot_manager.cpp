#include <algorithm>

#include "plot_manager.hpp"
#include "imgui.h"
#include "session.hpp"
#include "plot_tile.hpp"

void PlotManager::draw_tiles(const Session& session,
                             const std::map<std::uint8_t, std::string>& signal_registry) {
    ImGui::TextUnformatted("Plots");
    ImGui::Separator();

    if (ImGui::Button("Add plot")) {
        add_plot();
    }

    ImGui::Separator();

    int plot_to_remove = -1;

    for (std::size_t index = 0; index < plots_.size(); ++index) {
        if (plots_[index].draw(session, signal_registry, plots_.size() > 1, max_signals_per_plot)) {
            plot_to_remove = static_cast<int>(index);
        }
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

void to_json(json& j, const PlotManager& plot_manager) {
    j = json{{"plots", plot_manager.plots_}, {"next_plot_id", plot_manager.next_plot_id_}};
}

void from_json(const json& j, PlotManager& plot_manager) {
    plot_manager.plots_ = j.at("plots").get<std::vector<PlotTile>>();
    plot_manager.next_plot_id_ = j.at("next_plot_id").get<std::uint32_t>();
}
