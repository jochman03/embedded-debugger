#include "terminal.hpp"
#include "log_entry.hpp"
#include "imgui.h"

void draw_terminal_panel(SessionManager* session_manager, bool terminal_updated) {
    ImGui::TextUnformatted("Terminal");

    ImGui::SameLine();

    Session* selected_session = nullptr;
    if (session_manager != nullptr) {
        selected_session = session_manager->selected_session();
    }

    if (selected_session == nullptr) {
        return;
    }

    const float button_width =
        ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;

    const float button_x = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - button_width;

    ImGui::SetCursorPosX(button_x);
    if (ImGui::SmallButton("Clear")) {
        selected_session->logs().clear();
    }

    ImGui::Separator();

    ImGui::BeginChild("##inner_teminal", ImVec2(-1.0f, -1.0f), false);
    static bool follow_terminal = true;

    constexpr float bottom_threshold = 2.0f;

    const bool at_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - bottom_threshold;

    if (at_bottom) {
        follow_terminal = true;
    } else if (ImGui::IsWindowHovered() && (ImGui::GetIO().MouseWheel != 0.0f ||
                                            ImGui::IsMouseDragging(ImGuiMouseButton_Left))) {
        follow_terminal = false;
    }

    for (std::size_t i = 0; i < selected_session->logs().size(); i++) {
        const LogEntry& entry = selected_session->logs()[i];

        ImVec4 color = ImVec4(0.05f, 0.85f, 0.85f, 1.0f);
        switch (entry.type) {
            case LogType::Error:
                color = ImVec4(0.85f, 0.35f, 0.35f, 1.0f);
                break;
            case LogType::Warning:
                color = ImVec4(0.85f, 0.75f, 0.30f, 1.0f);
                break;
            case LogType::Info:
            case LogType::Reset:
                color = ImVec4(0.70f, 0.82f, 0.90f, 1.0f);
                break;
            default:
                break;
        }
        if (!entry.active) {
            color.x *= 0.6;
            color.y *= 0.6;
            color.z *= 0.6;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (entry.type == LogType::Reset) {
            ImGui::Text(" ");
        }
        ImGui::Text("[%u] %s [%s] %s", entry.timestamp, entry.prefix.c_str(), entry.tag.c_str(),
                    entry.message.c_str());

        ImGui::PopStyleColor();

        if (follow_terminal && terminal_updated) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}