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
    void request_registers();

    std::vector<std::string> ports;
    int selected_port{-1};
    SerialPort serial;
    ProtocolParser parser;

    ProfileManager profile_manager;

    Panel left_panel{Panel::Signals};
    double last_register_request_s_{-1.0};
    static constexpr double register_request_period_s_{1.0};
};