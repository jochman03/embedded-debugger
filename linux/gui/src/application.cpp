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

    // PANELS
    draw_side_panels(profile_manager, serial, parser, left_panel, ImVec2(right_width, available.y));
}
