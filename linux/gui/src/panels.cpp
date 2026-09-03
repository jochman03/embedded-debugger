#include <iostream>

#include "panels.hpp"
#include "protocol_command.hpp"
#include "colors.hpp"

#include "imgui.h"
#include "imgui_internal.h"

static void draw_signals_panel(ProfileManager& profile_manager, SerialPort& serial,
                               ProtocolParser& parser, Panel& left_panel);
static void draw_sessions_panel(ProfileManager& profile_manager, Panel& left_panel);
static void draw_profiles_panel(ProfileManager& profile_manager, SerialPort& serial,
                                Panel& left_panel);

static void draw_left_panel_button(const std::string name, Panel panel, Panel& left_panel);

static bool enter_key() {
    return ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
           ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
}

static bool esc_key() {
    return ImGui::IsKeyPressed(ImGuiKey_Escape, false);
}

void draw_side_panels(ProfileManager& profile_manager, SerialPort& serial, ProtocolParser& parser,
                      Panel& left_panel, ImVec2 size) {

    ImGui::BeginChild("SignalsPanel", ImVec2(size.x, size.y), true);

    if (ImGui::BeginTable("PanelTabs", 3, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        draw_left_panel_button("Signals", Panel::Signals, left_panel);

        ImGui::TableNextColumn();
        draw_left_panel_button("Sessions", Panel::Sessions, left_panel);

        ImGui::TableNextColumn();
        draw_left_panel_button("Profiles", Panel::Profiles, left_panel);

        ImGui::EndTable();
    }

    ImGui::Separator();
    draw_signals_panel(profile_manager, serial, parser, left_panel);
    draw_sessions_panel(profile_manager, left_panel);
    draw_profiles_panel(profile_manager, serial, left_panel);
    ImGui::EndChild();
}

static void draw_signals_panel(ProfileManager& profile_manager, SerialPort& serial,
                               ProtocolParser& parser, Panel& left_panel) {
    auto active_profile = profile_manager.active_profile();
    if (active_profile == nullptr) {
        return;
    }

    if (left_panel != Panel::Signals) {
        return;
    }
    SessionManager* session_manager = active_profile->session_manager();
    const Session* active_session = session_manager->active_session();
    const auto& signals = active_profile->signal_registry();

    if (signals.empty()) {
        ImGui::TextDisabled("N0 registered signals");

        if (serial.is_open()) {
            if (ImGui::Button("Send signal request")) {
                parser.clear();
                std::vector<std::uint8_t> request = make_command(0x10, {});
                if (!serial.write_bytes(request.data(), request.size())) {
                    std::cerr << "Failed to send register request: " << serial.error() << std::endl;
                }
            }
        }
        return;
    }

    if (ImGui::BeginTable("SignalsTable", 3)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Rate", ImGuiTableColumnFlags_WidthFixed, 75.0f);

        const double now = ImGui::GetTime();

        ImGui::TableHeadersRow();
        for (const auto& [signal_id, name] : signals) {
            ImGui::PushID(static_cast<int>(signal_id));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const float row_y = ImGui::GetCursorPosY();
            ImGui::Selectable("##signal", false, ImGuiSelectableFlags_SpanAllColumns);
            if (ImGui::BeginDragDropSource()) {
                const std::uint8_t payload_signal_id = signal_id;
                ImGui::SetDragDropPayload("PLOT_SIGNAL_ID", &payload_signal_id,
                                          sizeof(payload_signal_id));

                ImGui::TextUnformatted(name.c_str());
                ImGui::TextDisabled("Signal #%u", static_cast<unsigned>(signal_id));
                ImGui::EndDragDropSource();
            }
            const PlotSeries* series = nullptr;
            if (active_session != nullptr) {
                const auto it = active_session->getSeries().find(signal_id);
                if (it != active_session->getSeries().end()) {
                    series = &it->second;
                }
            }
            bool active = (series != nullptr) && (series->has_sample) &&
                          (series->frequency_hz > 0) &&
                          ((now - series->last_receive_time) <
                           std::max<double>(3.0 / series->frequency_hz, 0.1));

            ImGui::SetCursorPosY(row_y);
            ImGui::TableSetColumnIndex(0);

            ImGui::BeginDisabled(!active);
            ImGui::TextUnformatted(name.c_str());
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginDisabled(!active);
            if (series != nullptr) {
                ImGui::Text("%.2f", series->last_value);
            } else {
                ImGui::TextUnformatted("-");
            }
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(2);
            ImGui::BeginDisabled(!active);
            if (series != nullptr) {
                ImGui::Text("%.1f Hz", series->frequency_hz);
            } else {
                ImGui::TextUnformatted("-");
            }
            ImGui::EndDisabled();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

static void draw_sessions_panel(ProfileManager& profile_manager, Panel& left_panel) {

    if (left_panel != Panel::Sessions) {
        return;
    }

    SessionManager* session_manager{nullptr};
    auto active_profile = profile_manager.active_profile();
    if (active_profile != nullptr) {
        session_manager = active_profile->session_manager();
    }
    if (session_manager == nullptr || left_panel != Panel::Sessions) {
        return;
    }

    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_A, false) &&
        ImGui::GetIO().KeyCtrl) {
        session_manager->mark_all_sessions();
    }
    if (!ImGui::GetIO().WantTextInput && ImGui::GetTopMostPopupModal() == nullptr && esc_key()) {
        session_manager->clear_marked_sessions();
    }
    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        if (session_manager->marked_sessions().empty()) {
            const std::uint32_t selected = session_manager->selected_session_id();
            const std::uint32_t active = session_manager->active_session_id();
            if (selected != 0 && selected != active) {
                session_manager->toggle_marked_session(selected);
            }
        }
        if (!session_manager->marked_sessions().empty()) {
            ImGui::OpenPopup("Delete sessions");
        }
    }

    if (!ImGui::GetIO().WantTextInput && ImGui::GetTopMostPopupModal() == nullptr &&
        ImGui::GetIO().KeyShift) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) {
            session_manager->mark_session_neighbor(true);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) {
            session_manager->mark_session_neighbor(false);
        }
    }
    if (!ImGui::GetIO().WantTextInput && ImGui::GetTopMostPopupModal() == nullptr &&
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        session_manager->mark_session_for_save();
    }
    const auto& sessions = session_manager->session_list();

    if (sessions.empty()) {
        ImGui::TextDisabled("No sessions");
        return;
    }
    if (!session_manager->marked_sessions().empty()) {
        ImGui::Text("%zu selected", session_manager->marked_sessions().size());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);
        if (ImGui::SmallButton("Delete")) {
            ImGui::OpenPopup("Delete sessions");
        }
        ImGui::PopStyleColor(3);
        ImGui::Separator();
    }

    const ImGuiTableFlags table_flags =
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable("SessionsTable", 2, table_flags)) {
        return;
    }

    ImGui::TableSetupColumn("##Session", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Status", ImGuiTableColumnFlags_WidthFixed);

    for (const auto& session : sessions) {
        const bool selected = session.id == session_manager->selected_session_id();

        const bool active = session.id == session_manager->active_session_id();

        ImGui::PushID(static_cast<int>(session.id));
        ImGui::TableNextRow();

        const bool marked = session_manager->is_session_marked(session.id);
        if (selected || marked) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
        }

        ImGui::TableSetColumnIndex(0);

        const std::string session_name = "Session " + std::to_string(session.id);
        const ImVec2 cell_position = ImGui::GetCursorScreenPos();
        const ImVec2 cell_size(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());

        const bool clicked = ImGui::InvisibleButton("##SessionCell", cell_size);

        ImGui::GetWindowDrawList()->AddText(cell_position, ImGui::GetColorU32(ImGuiCol_Text),
                                            session_name.c_str());

        if (clicked) {
            if (ImGui::GetIO().KeyCtrl && !active) {
                session_manager->toggle_marked_session(session.id);
            } else {
                session_manager->clear_marked_sessions();
                session_manager->select_session(session.id);
            }
        }

        ImGui::TableSetColumnIndex(1);

        if (active) {
            const char* status = "Active";
            const float text_width = ImGui::CalcTextSize(status).x;
            const float available_width = ImGui::GetContentRegionAvail().x;
            const float offset = (available_width - text_width) * 0.5f;

            if (offset > 0.0f) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
            }

            ImGui::TextDisabled("%s", status);
        }

        ImGui::PopID();
    }

    ImGui::EndTable();

    if (ImGui::BeginPopupModal("Delete sessions", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto& marked = session_manager->marked_sessions();
        if (marked.size() == 1) {
            ImGui::Text("Are you sure you want to delete session %u?", *marked.begin());
        } else {
            ImGui::Text("Are you sure you want to delete %zu sessions?", marked.size());
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

        if (ImGui::Button("Delete", ImVec2(90.0f, 0.0f)) || enter_key()) {
            session_manager->remove_marked_sessions();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f)) || esc_key()) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    bool auto_save = session_manager->auto_save();
    if (ImGui::Checkbox("Auto-save sessions", &auto_save)) {
        session_manager->set_auto_save(auto_save);
    }
}

static void draw_profiles_panel(ProfileManager& profile_manager, SerialPort& serial,
                                Panel& left_panel) {
    if (left_panel != Panel::Profiles) {
        return;
    }

    const auto& profile_list = profile_manager.profile_list();
    const auto active_profile = profile_manager.active_profile();
    SessionManager* session_manager{nullptr};
    if (active_profile != nullptr) {
        session_manager = active_profile->session_manager();
    }
    if (profile_list.empty()) {
        ImGui::TextDisabled("No profiles");
        return;
    }

    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_A, false) &&
        ImGui::GetIO().KeyCtrl) {
        profile_manager.mark_all_profiles();
    }
    if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        if (!profile_manager.marked_profiles().empty()) {
            ImGui::OpenPopup("Delete profiles");
        }
    }
    if (!ImGui::GetIO().WantTextInput && ImGui::GetTopMostPopupModal() == nullptr && esc_key()) {
        profile_manager.clear_marked_profiles();
    }
    if (!ImGui::GetIO().WantTextInput && ImGui::GetTopMostPopupModal() == nullptr &&
        ImGui::GetIO().KeyShift) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) {
            profile_manager.mark_profile_neighbor(true);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) {
            profile_manager.mark_profile_neighbor(false);
        }
    }

    if (!profile_manager.marked_profiles().empty()) {
        ImGui::Text("%zu selected", profile_manager.marked_profiles().size());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);
        if (ImGui::SmallButton("Delete")) {
            ImGui::OpenPopup("Delete profiles");
        }
        ImGui::PopStyleColor(3);
        ImGui::Separator();
    }

    const ImGuiTableFlags table_flags =
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable("ProfilesTable", 2, table_flags)) {
        return;
    }

    ImGui::TableSetupColumn("##Profile", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Status", ImGuiTableColumnFlags_WidthFixed);

    bool profile_activated = false;

    for (const ProfileInfo& info : profile_list) {
        const bool active = (info.id == profile_manager.active_profile_id());

        ImGui::PushID(static_cast<int>(info.id));
        ImGui::TableNextRow();

        const bool marked = profile_manager.is_profile_marked(info.id);
        if (marked) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
        }

        ImGui::TableSetColumnIndex(0);

        if (active && active_profile != nullptr) {
            char name_buf[128];
            const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
            std::snprintf(name_buf, sizeof(name_buf), "%s", info.name.c_str());

            ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::transparent);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##name", name_buf, sizeof(name_buf), flags)) {
                active_profile->set_name(name_buf);
                active_profile->update_now();
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                active_profile->set_name(name_buf);
                active_profile->update_now();
                profile_manager.save_profile(info.id);
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        } else {
            const ImVec2 cell_position = ImGui::GetCursorScreenPos();
            const ImVec2 cell_size(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());
            const bool clicked = ImGui::InvisibleButton("##ProfileCell", cell_size);

            ImGui::GetWindowDrawList()->AddText(cell_position, ImGui::GetColorU32(ImGuiCol_Text),
                                                info.name.c_str());

            if (clicked) {
                if (ImGui::GetIO().KeyCtrl) {
                    profile_manager.toggle_marked_profile(info.id);
                } else {
                    profile_manager.clear_marked_profiles();
                }
            }
        }

        ImGui::TableSetColumnIndex(1);

        if (active) {
            const char* status = "Active";
            const float text_width = ImGui::CalcTextSize(status).x;
            const float available_width = ImGui::GetContentRegionAvail().x;
            const float offset = (available_width - text_width) * 0.5f;

            if (offset > 0.0f) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
            }
            ImGui::TextDisabled("%s", status);
        } else {
            if (ImGui::SmallButton("Activate")) {
                ImGui::OpenPopup("Activate profile");
            }
        }

        if (ImGui::BeginPopupModal("Activate profile", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to activate profile %s?", info.name.c_str());
            ImGui::Text("Active session and serial connection will be closed");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

            if (ImGui::Button("Activate", ImVec2(90.0f, 0.0f)) || enter_key()) {
                serial.close();
                if (session_manager != nullptr) {
                    session_manager->close_session();
                }
                if (profile_manager.activate_profile(info.id)) {
                    profile_activated = true;
                } else {
                    std::cerr << "Could not find profile with id = " << info.id << std::endl;
                }

                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f)) || esc_key()) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::PopID();
        if (profile_activated) {
            break;
        }
    }
    ImGui::EndTable();

    if (ImGui::BeginPopupModal("Delete profiles", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto& marked = profile_manager.marked_profiles();
        if (marked.size() == 1) {
            ImGui::Text("Are you sure you want to delete profile %u?", *marked.begin());
        } else {
            ImGui::Text("Are you sure you want to delete %zu profiles?", marked.size());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

        if (ImGui::Button("Delete", ImVec2(90.0f, 0.0f)) || enter_key()) {
            profile_manager.remove_marked_profiles();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f)) || esc_key()) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (profile_activated) {
        return;
    }

    if (ImGui::Button("New profile")) {
        profile_manager.create_profile();
    }
}

static void draw_left_panel_button(const std::string name, Panel panel, Panel& left_panel) {
    bool clicked{false};
    if (left_panel != panel) {
        ImGui::PushStyleColor(ImGuiCol_Button, colors::transparent);
    }
    if (ImGui::Button(name.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
        clicked = true;
    }
    if (left_panel != panel) {
        ImGui::PopStyleColor();
    }
    if (clicked) {
        left_panel = panel;
    }
}
