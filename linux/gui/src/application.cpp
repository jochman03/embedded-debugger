#include <string>
#include <vector>
#include <iostream>

#include "application.hpp"
#include "serial_port.hpp"
#include "serial_scan.hpp"
#include "protocol_parser.hpp"
#include "protocol_command.hpp"
#include "log_entry.hpp"
#include "log_type.hpp"
#include "imgui.h"
#include "plot_manager.hpp"
#include "terminal.hpp"
#include "session.hpp"
#include "colors.hpp"

Application::Application() {
    ports = scan_serial_ports();
    selected_port = ports.empty() ? -1 : 0;

    profile_manager.load_profiles();
    if (profile_manager.profiles().empty()) {
        profile_manager.create_profile();
        profile_manager.activate_profile(1);
    }
}

void Application::update() {

    SessionManager* session_manager{nullptr};
    Session* active_session{nullptr};
    PlotManager* plots{nullptr};
    auto active_profile = profile_manager.active_profile();
    if (active_profile != nullptr) {
        session_manager = active_profile->session_manager();
        active_session = session_manager->active_session();
        plots = active_profile->plot_manager();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("Main UI", nullptr, flags);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 9.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    ImGui::BeginChild("TopBar", ImVec2(0.0f, 52.0f), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Port:");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(320.0f);

    const char* selected_name = selected_port >= 0 && selected_port < static_cast<int>(ports.size())
                                    ? ports[selected_port].c_str()
                                    : "No ports found";

    if (ImGui::BeginCombo("##serial_port", selected_name)) {
        for (int i = 0; i < static_cast<int>(ports.size()); ++i) {
            const bool selected = selected_port == i;

            if (ImGui::Selectable(ports[i].c_str(), selected)) {
                selected_port = i;
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();

    if (ImGui::Button("Scan", ImVec2(90.0f, 0.0f))) {
        ports = scan_serial_ports();
        selected_port = ports.empty() ? -1 : 0;
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(selected_port < 0);

    ImGui::SameLine();

    const bool valid_port = selected_port >= 0 && selected_port < static_cast<int>(ports.size());

    if (serial.is_open()) {
        if (ImGui::Button("Disconnect", ImVec2(100.0f, 0.0f))) {
            serial.close();
            if (session_manager != nullptr) {
                session_manager->close_session();
            }
        }
    } else {
        ImGui::BeginDisabled(!valid_port);

        if (ImGui::Button("Connect", ImVec2(100.0f, 0.0f))) {
            if (serial.open(ports[selected_port], 115200)) {
                parser.clear();

                std::vector<std::uint8_t> request = make_command(0x10, {});
                if (!serial.write_bytes(request.data(), request.size())) {
                    std::cerr << "Failed to send register request: " << serial.error() << std::endl;
                }
                if (session_manager != nullptr) {
                    session_manager->start_session();
                }
            }
        }

        ImGui::EndDisabled();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (serial.is_open()) {
        ImGui::TextUnformatted("Connected");
    } else {
        ImGui::TextDisabled("Disconnected");
    }

    ImGui::EndChild();

    bool received_data = false;
    if (session_manager != nullptr) {
        active_session = session_manager->active_session();
    }
    if (active_session != nullptr && session_manager != nullptr && serial.is_open()) {
        const std::vector<std::uint8_t> data = serial.read_available();

        if (!data.empty()) {
            const std::vector<ParsedFrame> frames = parser.push(data);

            for (std::size_t i = 0; i < frames.size(); ++i) {
                const ParsedFrame& frame = frames[i];

                if (frame.type == FrameType::Log) {

                    LogEntry entry;
                    entry.timestamp = frame.timestamp;
                    entry.prefix = frame.prefix;
                    entry.tag = frame.tag;
                    entry.message = frame.message;
                    entry.type = prefix_to_log_type(frame.prefix);
                    entry.active = true;
                    active_session->logs().push_back(entry);
                    if (entry.type == LogType::Reset) {
                        session_manager->close_session();
                        session_manager->start_session();
                        active_session = session_manager->active_session();
                    }

                    received_data = true;
                }

                if (frame.type == FrameType::Register) {
                    active_session->register_signal(frame.signal_id, frame.name);
                    received_data = true;
                }
                if (frame.type == FrameType::Plot) {
                    active_session->add_sample(frame.signal_id, frame.name, frame.timestamp,
                                               frame.value);
                }
            }
        }
    }
    if (plots != nullptr) {
        plots->update_recorders();
    }
    draw_workspace(received_data);

    ImGui::PopStyleVar(4);

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void Application::draw_workspace(bool terminal_updated) {
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    const float usable_width = available.x - spacing * 2.0f;
    const float left_width = usable_width * 0.2f;
    const float right_width = usable_width * 0.2f;
    const float center_width = usable_width - left_width - right_width;

    SessionManager* session_manager{nullptr};
    PlotManager* plots{nullptr};
    auto active_profile = profile_manager.active_profile();
    if (active_profile != nullptr) {
        session_manager = active_profile->session_manager();
        plots = active_profile->plot_manager();
    }

    Session* selected_session = nullptr;
    if (session_manager != nullptr) {
        selected_session = session_manager->selected_session();
    }

    ImGui::BeginChild("TerminalPanel", ImVec2(left_width, available.y), true);

    draw_terminal_panel(session_manager, terminal_updated);

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("PlotsPanel", ImVec2(center_width, available.y), true);

    if (selected_session != nullptr && plots != nullptr) {
        plots->draw_tiles(*selected_session);
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("SignalsPanel", ImVec2(right_width, available.y), true);

    if (ImGui::BeginTable("PanelTabs", 3, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        draw_left_panel_button("Signals", Panel::Signals);

        ImGui::TableNextColumn();
        draw_left_panel_button("Sessions", Panel::Sessions);

        ImGui::TableNextColumn();
        draw_left_panel_button("Profiles", Panel::Profiles);

        ImGui::EndTable();
    }

    ImGui::Separator();
    draw_signals_panel();
    draw_sessions_panel();
    draw_profiles_panel();
    ImGui::EndChild();
}

void Application::draw_signals_panel() {
    SessionManager* session_manager{nullptr};
    auto active_profile = profile_manager.active_profile();
    if (active_profile != nullptr) {
        session_manager = active_profile->session_manager();
    }

    if (session_manager == nullptr) {
        return;
    }
    if (session_manager->active_session() == nullptr) {
        return;
    }

    if (left_panel != Panel::Signals) {
        return;
    }
    auto series_ = session_manager->active_session()->getSeries();

    if (series_.empty()) {
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
        for (const auto& [signal_id, series] : series_) {
            ImGui::PushID(static_cast<int>(signal_id));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const float row_y = ImGui::GetCursorPosY();
            ImGui::Selectable("##signal", false, ImGuiSelectableFlags_SpanAllColumns);
            if (ImGui::BeginDragDropSource()) {
                const std::uint8_t payload_signal_id = signal_id;
                ImGui::SetDragDropPayload("PLOT_SIGNAL_ID", &payload_signal_id,
                                          sizeof(payload_signal_id));

                ImGui::TextUnformatted(series.name.c_str());
                ImGui::TextDisabled("Signal #%u", static_cast<unsigned>(signal_id));
                ImGui::EndDragDropSource();
            }
            bool active = series.has_sample && (series.frequency_hz > 0) &&
                          ((now - series.last_receive_time) <
                           std::max<double>(3.0 / series.frequency_hz, 0.1));

            ImGui::SetCursorPosY(row_y);
            ImGui::TableSetColumnIndex(0);

            ImGui::BeginDisabled(!active);
            ImGui::TextUnformatted(series.name.c_str());
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginDisabled(!active);
            ImGui::Text("%.2f", series.last_value);
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(2);
            ImGui::BeginDisabled(!active);

            ImGui::Text("%.1f Hz", series.frequency_hz);
            ImGui::EndDisabled();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void Application::draw_sessions_panel() {

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

    const auto& sessions = session_manager->sessions();

    if (sessions.empty()) {
        ImGui::TextDisabled("No sessions");
        return;
    }

    std::uint32_t session_to_remove = 0;

    const ImGuiTableFlags table_flags =
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable("SessionsTable", 3, table_flags)) {
        return;
    }

    ImGui::TableSetupColumn("##Session", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableSetupColumn("##Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);

    ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed, 32.0f);

    for (const auto& session : sessions) {
        const bool selected = session.id() == session_manager->selected_session_id();

        const bool active = session.id() == session_manager->active_session_id();

        ImGui::PushID(static_cast<int>(session.id()));
        ImGui::TableNextRow();

        if (selected) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
        }

        ImGui::TableSetColumnIndex(0);

        const std::string session_name = "Session " + std::to_string(session.id());
        const ImVec2 cell_position = ImGui::GetCursorScreenPos();
        const ImVec2 cell_size(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight());

        const bool clicked = ImGui::InvisibleButton("##SessionCell", cell_size);

        ImGui::GetWindowDrawList()->AddText(cell_position, ImGui::GetColorU32(ImGuiCol_Text),
                                            session_name.c_str());

        if (clicked) {
            session_manager->select_session(session.id());
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

        ImGui::TableSetColumnIndex(2);

        ImGui::BeginDisabled(active);
        ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

        if (ImGui::SmallButton("-")) {
            ImGui::OpenPopup("Delete session");
        }

        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("Delete session", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete session %u?", session.id());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

            if (ImGui::Button("Delete", ImVec2(90.0f, 0.0f))) {
                session_to_remove = session.id();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndTable();

    if (session_to_remove != 0) {
        session_manager->remove_session(session_to_remove);
    }
}

void Application::draw_profiles_panel() {
    if (left_panel != Panel::Profiles) {
        return;
    }

    auto& profiles = profile_manager.profiles();
    const auto active_profile = profile_manager.active_profile();
    SessionManager* session_manager{nullptr};
    if (active_profile != nullptr) {
        session_manager = active_profile->session_manager();
    }
    if (profiles.empty()) {
        ImGui::TextDisabled("No profiles");
        return;
    }

    std::uint32_t profile_to_remove = 0;

    const ImGuiTableFlags table_flags =
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable("ProfilesTable", 3, table_flags)) {
        return;
    }

    ImGui::TableSetupColumn("##Profile", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed, 32.0f);

    for (auto& profile : profiles) {
        const bool active = (profile.id() == active_profile->id());

        ImGui::PushID(static_cast<int>(profile.id()));
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        const std::string& profile_name = profile.name();
        char name_buf[128];

        const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
        std::snprintf(name_buf, sizeof(name_buf), "%s", profile_name.c_str());

        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::transparent);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##name", name_buf, sizeof(name_buf), flags)) {
            profile.set_name(name_buf);
            profile.update_now();
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            profile.set_name(name_buf);
            profile.update_now();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

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

        ImGui::TableSetColumnIndex(2);

        ImGui::BeginDisabled(active);

        ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

        if (ImGui::SmallButton("-")) {
            ImGui::OpenPopup("Delete profile");
        }
        ImGui::PopStyleColor(3);

        ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("Delete profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete profile %s?", profile_name.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

            if (ImGui::Button("Delete", ImVec2(90.0f, 0.0f))) {
                profile_to_remove = profile.id();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Activate profile", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to activate profile %s?", profile_name.c_str());
            ImGui::Text("Active session and serial connection will be closed");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, colors::red);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::dark_red);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::dark_red);

            if (ImGui::Button("Activate", ImVec2(90.0f, 0.0f))) {
                if (active_profile != nullptr && session_manager != nullptr) {
                    serial.close();
                    session_manager->close_session();
                    if (profile_manager.activate_profile(profile.id())) {

                    } else {
                        std::cerr << "Could not find profile with id = " << profile.id()
                                  << std::endl;
                    }
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(90.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
    if (ImGui::Button("New profile")) {
        profile_manager.create_profile();
    }
    if (ImGui::Button("save profile")) {
        profile_manager.save_profile(profile_manager.active_profile_id());
    }
    if (profile_to_remove != 0) {
        profile_manager.remove_profile(profile_to_remove);
    }
}

void Application::draw_left_panel_button(const std::string name, Panel panel) {
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
