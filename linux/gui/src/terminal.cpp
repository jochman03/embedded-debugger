#include <iostream>

#include "terminal.hpp"
#include "log_entry.hpp"
#include "serial_port.hpp"
#include "protocol_command.hpp"
#include "protocol.hpp"

#include "imgui.h"

static std::vector<std::string> command_history;
static int command_history_pos = -1;
static char command_line[256] = {};

static int command_input_callback(ImGuiInputTextCallbackData* data);

void draw_terminal_panel(SessionManager* session_manager, SerialPort& serial,
                         bool terminal_updated) {
    ImGui::TextUnformatted("Terminal");

    ImGui::SameLine();

    Session* selected_session = nullptr;
    Session* active_session = nullptr;

    if (session_manager != nullptr) {
        selected_session = session_manager->selected_session();
        active_session = session_manager->active_session();
    }

    const bool active_terminal = selected_session != nullptr && selected_session == active_session;

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

    const float input_height = ImGui::GetFrameHeightWithSpacing();

    ImGui::BeginChild("##inner_teminal", ImVec2(-1.0f, -input_height), false);
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

    ImGui::BeginDisabled(!serial.is_open() || !active_terminal);
    static bool focus_command_line{false};

    if (focus_command_line) {
        ImGui::SetKeyboardFocusHere();
        focus_command_line = false;
    }

    ImGui::SetNextItemWidth(-1.0f);
    const bool submitted =
        ImGui::InputText("##command_line", command_line, sizeof(command_line),
                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                         command_input_callback);
    ImGui::EndDisabled();

    if (submitted && command_line[0] != '\0' && active_terminal && active_session != nullptr) {

        FrameCommand frame{};
        if (!parse_frame_command(command_line, frame)) {
            active_session->add_log(LogType::Error, "GUI", "Invalid command syntax.",
                                    ImGui::GetTime());
            focus_command_line = true;
        } else {
            const auto request = encode_frame_command(frame);
            if (!serial.write_bytes(request.data(), request.size())) {
                active_session->add_log(LogType::Error, "GUI", "Failed to send command.",
                                        ImGui::GetTime());
            } else {
                active_session->add_log(LogType::Info, "GUI", std::string("> ") + command_line,
                                        ImGui::GetTime());
                if (command_history.empty() || command_history.back() != command_line) {
                    command_history.push_back(command_line);
                }
                command_history_pos = -1;
            }
            focus_command_line = true;
        }
        command_line[0] = '\0';
    }
}

static int command_input_callback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory) {
        return 0;
    }
    if (command_history.empty()) {
        return 0;
    }

    if (data->EventKey == ImGuiKey_UpArrow) {
        if (command_history_pos == -1) {
            command_history_pos = static_cast<int>(command_history.size()) - 1;
        } else if (command_history_pos > 0) {
            command_history_pos--;
        }
    } else if (data->EventKey == ImGuiKey_DownArrow) {
        if (command_history_pos >= 0) {
            command_history_pos++;
            if (command_history_pos >= static_cast<int>(command_history.size())) {
                command_history_pos = -1;
                data->DeleteChars(0, data->BufTextLen);
                return 0;
            }
        }
    } else {
        return 0;
    }
    if (command_history_pos >= 0) {
        const std::string& line = command_history[static_cast<std::size_t>(command_history_pos)];
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, line.c_str());
    }
    return 0;
}