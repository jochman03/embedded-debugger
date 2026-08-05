#include "application.hpp"
#include <string>
#include <vector>

#include "serial_port.hpp"
#include "serial_scan.hpp"
#include "protocol_parser.hpp"
#include "protocol_command.hpp"
#include "log_entry.hpp"
#include "imgui.h"
#include "plot_manager.hpp"
#include "terminal.hpp"
#include "session.hpp"

Application::Application() {
    ports = scan_serial_ports();
    selected_port = ports.empty() ? -1 : 0;
}

void Application::update() {

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
            session_manager.close_session();
        }
    } else {
        ImGui::BeginDisabled(!valid_port);

        if (ImGui::Button("Connect", ImVec2(100.0f, 0.0f))) {
            if (serial.open(ports[selected_port], 115200)) {
                std::vector<std::uint8_t> request = make_command(Command::RequestRegisters, {});
                if (!serial.write_bytes(request.data(), request.size())) {
                    std::fprintf(stderr, "Failed to send register request: %s\n",
                                 serial.error().c_str());
                } else {
                    std::fprintf(stderr, "Register request sent\n");
                }
                session_manager.start_session();
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
    auto active_session = session_manager.active_session();

    if (active_session != nullptr && serial.is_open()) {
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

                    logs.push_back(entry);
                    if (entry.type == LogType::Reset) {
                        deactivate_entries(logs);
                        session_manager.close_session();
                        session_manager.start_session();
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
    plots.update_recorders();
    draw_workspace(plots, logs, received_data);

    ImGui::PopStyleVar(4);

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void Application::deactivate_entries(std::vector<LogEntry>& entries) {
    bool found_latest_restart = false;

    for (int i = (entries.size() - 1); i > -1; i--) {
        LogEntry& entry = entries[i];
        if (found_latest_restart) {
            entry.active = false;
            continue;
        }
        if (entry.type == LogType::Reset) {
            found_latest_restart = true;
        }
    }
}

void Application::draw_workspace(PlotManager& plots, std::vector<LogEntry>& logs,
                                 bool terminal_updated) {
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    const float usable_width = available.x - spacing * 2.0f;
    const float left_width = usable_width * 0.2f;
    const float right_width = usable_width * 0.2f;
    const float center_width = usable_width - left_width - right_width;

    auto selected_session = session_manager.selected_session();

    ImGui::BeginChild("TerminalPanel", ImVec2(left_width, available.y), true);

    draw_terminal_panel(logs, terminal_updated);

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("PlotsPanel", ImVec2(center_width, available.y), true);

    if (selected_session != nullptr) {
        plots.draw_tiles(*selected_session);
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("SignalsPanel", ImVec2(right_width, available.y), true);

    if (ImGui::BeginTable("PanelTabs", 3, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        if (ImGui::Button("Signals", ImVec2(-FLT_MIN, 0.0f))) {
            left_panel = Panel::Signals;
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Sessions", ImVec2(-FLT_MIN, 0.0f))) {
            left_panel = Panel::Sessions;
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Profiles", ImVec2(-FLT_MIN, 0.0f))) {
            left_panel = Panel::Profiles;
        }

        ImGui::EndTable();
    }

    ImGui::Separator();
    draw_signals_panel();
    draw_sessions_panel();
    draw_profiles_panel();
    ImGui::EndChild();
}

void Application::draw_signals_panel() {
    if (session_manager.active_session() == nullptr) {
        return;
    }

    if (left_panel != Panel::Signals) {
        return;
    }
    auto series_ = session_manager.active_session()->getSeries();

    if (series_.empty()) {
        ImGui::TextDisabled("NO registered signals");

        if (serial.is_open()) {
            if (ImGui::Button("Send signal request")) {
                std::vector<std::uint8_t> request = make_command(Command::RequestRegisters, {});
                if (!serial.write_bytes(request.data(), request.size())) {
                    std::fprintf(stderr, "Failed to send register request: %s\n",
                                 serial.error().c_str());
                } else {
                    std::fprintf(stderr, "Register request sent\n");
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

    const auto& sessions = session_manager.sessions();

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
        const bool selected = session.id() == session_manager.selected_session_id();

        const bool active = session.id() == session_manager.active_session_id();

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
            session_manager.select_session(session.id());
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

        if (ImGui::SmallButton("-")) {
            ImGui::OpenPopup("Delete session");
        }

        ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("Delete session", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete session %u?", session.id());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Delete", ImVec2(90.0f, 0.0f))) {
                session_to_remove = session.id();
                ImGui::CloseCurrentPopup();
            }

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
        session_manager.remove_session(session_to_remove);
    }
}

void Application::draw_profiles_panel() {}
