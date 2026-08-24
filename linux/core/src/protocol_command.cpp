#include <cstddef>
#include <limits>
#include <string>
#include <cstdint>
#include <iomanip>
#include <string_view>
#include <vector>
#include <sstream>

#include "protocol.hpp"
#include "protocol_command.hpp"

static bool parse_command(std::string_view text, std::uint8_t& out);

std::vector<std::uint8_t> encode_frame_command(const FrameCommand& command) {
    return make_command(command.type, command.payload);
}

bool parse_frame_command(std::string_view line, FrameCommand& out) {
    out = {};

    std::istringstream iss{std::string(line)};

    std::string type;
    if (!(iss >> type)) {
        return false;
    }
    if (!parse_command(type, out.type)) {
        return false;
    }

    std::string byte;
    while (iss >> byte) {
        std::uint8_t value = 0;
        if (!parse_command(byte, value)) {
            return false;
        }
        out.payload.push_back(value);
    }
    return true;
}

std::vector<std::uint8_t> make_command(uint8_t type, const std::vector<std::uint8_t>& payload) {
    if (payload.size() > std::numeric_limits<std::uint8_t>::max()) {
        return {};
    }

    std::vector<std::uint8_t> frame;
    frame.reserve(payload.size() + 5);

    frame.push_back(protocol::sync1);
    frame.push_back(protocol::sync2);
    frame.push_back(type);
    frame.push_back(static_cast<std::uint8_t>(payload.size()));

    frame.insert(frame.end(), payload.begin(), payload.end());
    const std::uint8_t crc = protocol::calculate_crc8(frame.data() + 2, frame.size() - 2);

    frame.push_back(crc);

    return frame;
}

std::string format_hex_bytes(const std::vector<std::uint8_t>& data) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');

    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned>(data[i]);
    }
    return out.str();
}

static bool parse_command(std::string_view text, std::uint8_t& out) {
    if (text.empty()) {
        return false;
    }
    try {
        const unsigned long value = std::stoul(std::string(text), nullptr, 0);
        if (value > 0xFFUL) {
            return false;
        }
        out = static_cast<std::uint8_t>(value);
        return true;
    } catch (...) {
        return false;
    }
}
