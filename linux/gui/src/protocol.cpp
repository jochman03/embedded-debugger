#include "protocol.hpp"

namespace protocol {

std::uint8_t calculate_crc8(const std::uint8_t* data, std::size_t size) {
    std::uint8_t crc = 0;

    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80) != 0) {
                crc = static_cast<std::uint8_t>((crc << 1) ^ 0x07);
            } else {
                crc = static_cast<std::uint8_t>(crc << 1);
            }
        }
    }

    return crc;
}

} // namespace protocol