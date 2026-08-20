#include "serial_scan.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<std::string> scan_serial_ports() {
    std::vector<std::string> ports;
    std::error_code error;

    const fs::path by_id = "/dev/serial/by-id";

    if (fs::exists(by_id, error)) {
        for (const auto& entry : fs::directory_iterator(by_id)) {
            ports.push_back(entry.path().string());
        }
    }

    if (ports.empty()) {
        for (const auto& entry : fs::directory_iterator("/dev")) {
            const std::string name = entry.path().filename().string();

            if (name.rfind("ttyACM", 0) == 0 || name.rfind("ttyUSB", 0) == 0) {
                ports.push_back(entry.path().string());
            }
        }
    }

    std::sort(ports.begin(), ports.end());

    return ports;
}