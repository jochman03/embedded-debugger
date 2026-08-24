#pragma once

#include <array>
#include <cstdint>
#include <vector>

struct FrameCommand {
    std::uint8_t type;
    std::vector<std::uint8_t> payload;
};

std::vector<std::uint8_t> encode_frame_command(const FrameCommand& command);

bool parse_frame_command(std::string_view line, FrameCommand& out);
std::vector<std::uint8_t> make_command(std::uint8_t type,
                                       const std::vector<std::uint8_t>& payload = {});

std::string format_hex_bytes(const std::vector<std::uint8_t>& data);