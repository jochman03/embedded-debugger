#pragma once

#include <array>
#include <cstdint>
#include <vector>

enum class Command : std::uint8_t { RequestRegisters = 0x01 };

std::vector<std::uint8_t> make_command(Command command,
                                       const std::vector<std::uint8_t>& payload = {});