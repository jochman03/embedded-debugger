#pragma once

#include <cstddef>
#include <cstdint>

namespace protocol {

inline constexpr std::uint8_t sync1 = 0xA0;
inline constexpr std::uint8_t sync2 = 0x0A;

std::uint8_t calculate_crc8(const std::uint8_t* data, std::size_t size);

} // namespace protocol