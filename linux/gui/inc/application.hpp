#pragma once

#include <vector>
#include <string>

#include "log_entry.hpp"
#include "plot_manager.hpp"
#include "serial_port.hpp"
#include "protocol_parser.hpp"
#include "session_manager.hpp"

class Application {
  public:
    Application();
    void update();

  private:
    void deactivate_entries(std::vector<LogEntry>& entries);
    void draw_workspace(PlotManager& plots, std::vector<LogEntry>& logs, bool terminal_updated);

    void draw_signals_panel();
    void draw_sessions_panel();
    void draw_profiles_panel();

    std::vector<std::string> ports;
    int selected_port = -1;
    SerialPort serial;
    ProtocolParser parser;
    std::vector<LogEntry> logs;
    PlotManager plots;
    SessionManager session_manager;
    Panel left_panel{Panel::Signals};
};