#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <poll.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "log_entry.hpp"
#include "protocol_command.hpp"
#include "protocol_parser.hpp"
#include "serial_port.hpp"
#include "serial_scan.hpp"
#include "terminal_colors.hpp"

bool running = true;
std::optional<std::string> pending_command;

void handle_command(char *line) {
  if (line == nullptr) {
    running = false;
    return;
  }

  std::string command{line};

  if (!command.empty()) {
    add_history(line);
  }

  std::free(line);
  pending_command = command;
}

int main(int argc, char *argv[]) {
  static SerialPort serial;
  static ProtocolParser parser;
  static std::vector<LogEntry> logs;

  while (running) {
    const std::vector<std::string> ports = scan_serial_ports();
    if (ports.empty()) {
      std::cout << terminal_color::red << "No serial ports found."
                << terminal_color::reset << std::endl;
      std::cout << "Press enter to scan again." << std::endl;
      std::cin.get();
      continue;
    }

    std::size_t index = 1;
    std::size_t selected_port{0};

    while (selected_port < 1 || selected_port > ports.size()) {
      std::cout << "Select port:" << std::endl;
      index = 1;
      for (const auto &port : ports) {
        std::cout << terminal_color::gray << index << ". "
                  << terminal_color::reset << port << std::endl;
        index++;
      }

      std::cin >> selected_port;
    }
    selected_port = selected_port - 1;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (!serial.open(ports[selected_port], 115200)) {
      std::cout << terminal_color::red
                << "Could not open port: " << ports[selected_port]
                << terminal_color::reset << std::endl;
      return 1;
    }

    rl_callback_handler_install("> ", handle_command);
    pollfd input{};
    input.fd = STDIN_FILENO;
    input.events = POLLIN;

    while (running && serial.is_open()) {
      const int poll_result = poll(&input, 1, 0);

      if (poll_result > 0 && (input.revents & POLLIN)) {
        rl_callback_read_char();
      }

      const std::vector<std::uint8_t> data = serial.read_available();

      if (!data.empty()) {
        const std::vector<ParsedFrame> frames = parser.push(data);

        for (std::size_t i = 0; i < frames.size(); ++i) {
          const ParsedFrame &frame = frames[i];

          if (frame.type == FrameType::Log) {
            auto log_type = prefix_to_log_type(frame.prefix);
            auto log_color = terminal_color::reset;

            switch (log_type) {
            case LogType::Error:
              log_color = terminal_color::red;
              break;
            case LogType::Warning:
              log_color = terminal_color::yellow;
              break;
            case LogType::Reset:
              log_color = terminal_color::gray;
              break;
            default:
              break;
            }

            if (log_type == LogType::Reset) {
              std::cout << std::endl;
              std::cout << std::endl;
            }

            std::ostringstream output;

            output << log_color << "[ " << frame.timestamp << " ] "
                   << "[ " << frame.prefix << " ] "
                   << "[ " << frame.tag << " ] " << frame.message
                   << terminal_color::reset;

            std::cout << "\r\033[2K";
            std::cout << output.str() << '\n' << std::flush;

            rl_on_new_line();
            rl_redisplay();
          }
        }
      }
      if (pending_command.has_value()) {
        std::string command = pending_command.value();
        if (command == "quit") {
          running = false;
        } else if (command == "help") {
          std::cout << terminal_color::green << "*** HELP ***"
                    << terminal_color::reset << "\n";
          std::cout << " Commands:\n";
          std::cout
              << "  - help                     <-  prints available commands\n";
          std::cout << "  - quit                     <-  quits application\n";
          std::cout
              << "  - dc                       <-  disconnects from serial "
                 "port\n";
          std::cout << "  - <command> <parameters>   <-  sends user command to "
                       "target "
                       "device\n";
          std::cout << terminal_color::green << "*** ***"
                    << terminal_color::reset << std::endl;

        } else if (command == "dc") {
          serial.close();
        } else {
          auto request = make_user_command(command);
          if (!serial.write_bytes(request.data(), request.size())) {
            std::cerr << terminal_color::red
                      << "Failed to send command: " << serial.error()
                      << terminal_color::reset << '\n';
          }
        }
        pending_command.reset();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    rl_callback_handler_remove();
    std::cout << terminal_color::yellow << "Serial disconnected."
              << terminal_color::reset << std::endl;
  }

  return 0;
}