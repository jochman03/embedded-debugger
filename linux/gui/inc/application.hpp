#pragma once

#include <vector>
#include <string>

#include "serial_port.hpp"
#include "protocol_parser.hpp"
#include "profile_manager.hpp"
#include "panels.hpp"

class Application {
  public:
    Application();
    void update();
    void shutdown();

  private:
    void draw_workspace(bool terminal_updated);
    std::vector<std::string> ports;
    int selected_port{-1};
    SerialPort serial;
    ProtocolParser parser;

    ProfileManager profile_manager;

    Panel left_panel{Panel::Signals};
};