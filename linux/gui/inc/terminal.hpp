#pragma once

#include <vector>
#include "log_entry.hpp"
#include "session_manager.hpp"

class SerialPort;

void draw_terminal_panel(SessionManager* session_manager, SerialPort& serial,
                         bool terminal_updated);