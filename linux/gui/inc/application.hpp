#pragma once

#include <vector>
#include <string>

#include "log_entry.hpp"
#include "plot_manager.hpp"
#include "serial_port.hpp"
#include "protocol_parser.hpp"
#include "session_manager.hpp"
#include "profile_manager.hpp"

class Application {
  public:
    Application();
    void update();

  private:
    void draw_workspace(bool terminal_updated);

    void draw_signals_panel();
    void draw_sessions_panel();
    void draw_profiles_panel();

    void draw_left_panel_button(const std::string name, Panel panel);
    std::vector<std::string> ports;
    int selected_port{-1};
    SerialPort serial;
    ProtocolParser parser;

    Panel left_panel{Panel::Signals};

    ProfileManager profile_manager;
};