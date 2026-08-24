#pragma once

#include <cstddef>
#include <cstdint>

namespace protocol {

inline constexpr std::uint8_t sync1 = 0xA0;
inline constexpr std::uint8_t sync2 = 0x0A;

inline constexpr std::uint8_t cmd_request_registers = 0x10;
inline constexpr std::uint8_t cmd_set_led = 0x11;

std::uint8_t calculate_crc8(const std::uint8_t* data, std::size_t size);

} // namespace protocol