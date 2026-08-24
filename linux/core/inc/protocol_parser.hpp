#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

enum class FrameType : std::uint8_t { Register = 0, Log, Plot };

enum class SignalType : std::uint8_t { Uint8 = 0, Uint16, Uint32, Int8, Int16, Int32, Float };

enum class ParserState { Sync1, Sync2, Command, Length, Payload, Crc };

struct SignalInfo {
    std::string name;
    SignalType type;
    std::size_t signal_size;
};

struct ParsedFrame {
    std::uint8_t command = 0;
    std::vector<std::uint8_t> payload;

    FrameType type;
    std::uint8_t signal_id = 0;
    SignalType signal_type = SignalType::Uint8;
    std::uint32_t timestamp = 0;
    double value = 0.0;

    std::string name;
    std::string prefix;
    std::string tag;
    std::string message;
};

class ProtocolParser {
  public:
    void clear();

    std::vector<ParsedFrame> push(const std::vector<std::uint8_t>& data);

  private:
    void reset();

    std::size_t signal_size(SignalType type) const;

    std::uint16_t read_uint16(std::size_t offset) const;
    std::uint32_t read_uint32(std::size_t offset) const;
    float read_float(std::size_t offset) const;

    ParserState state_{ParserState::Sync1};
    std::uint8_t type_{};
    std::uint8_t length_{};
    std::uint8_t payload_index_{};
    std::array<std::uint8_t, 255> payload_{};
    std::unordered_map<std::uint8_t, SignalInfo> signals_;
    ParsedFrame decode_frame();
};