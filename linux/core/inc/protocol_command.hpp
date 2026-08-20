#pragma once

#include <array>
#include <cstdint>
#include <vector>

std::vector<std::uint8_t> make_command(std::uint8_t type,
                                       const std::vector<std::uint8_t>& payload = {});
