#include "protocol.hpp"
#include "protocol_command.hpp"

#include <cstddef>
#include <limits>

std::vector<std::uint8_t> make_command(Command command, const std::vector<std::uint8_t>& payload) {
    if (payload.size() > std::numeric_limits<std::uint8_t>::max()) {
        return {};
    }

    std::vector<std::uint8_t> frame;
    frame.reserve(payload.size() + 5);

    frame.push_back(protocol::sync1);
    frame.push_back(protocol::sync2);
    frame.push_back(static_cast<std::uint8_t>(command));
    frame.push_back(static_cast<std::uint8_t>(payload.size()));

    frame.insert(frame.end(), payload.begin(), payload.end());
    const std::uint8_t crc = protocol::calculate_crc8(frame.data() + 2, frame.size() - 2);

    frame.push_back(crc);

    return frame;
}
