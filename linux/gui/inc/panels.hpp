#pragma once

#include "profile_manager.hpp"
#include "serial_port.hpp"
#include "protocol_parser.hpp"

struct ImVec2;

enum class Panel { Signals, Sessions, Profiles };

void draw_side_panels(ProfileManager& profile_manager, SerialPort& serial, ProtocolParser& parser,
                      Panel& left_panel, ImVec2 size);