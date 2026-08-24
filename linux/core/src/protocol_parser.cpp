#include <cstring>

#include "protocol.hpp"
#include "protocol_parser.hpp"

void ProtocolParser::reset() {
    state_ = ParserState::Sync1;
    payload_index_ = 0;
}

std::uint16_t ProtocolParser::read_uint16(std::size_t offset) const {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(payload_[offset]) |
                                      (static_cast<std::uint16_t>(payload_[offset + 1]) << 8));
}
std::uint32_t ProtocolParser::read_uint32(std::size_t offset) const {
    return static_cast<std::uint32_t>(payload_[offset]) |
           (static_cast<std::uint32_t>(payload_[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(payload_[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(payload_[offset + 3]) << 24);
}
float ProtocolParser::read_float(std::size_t offset) const {
    const std::uint32_t raw_value = read_uint32(offset);

    float value = 0.0f;

    std::memcpy(&value, &raw_value, sizeof(value));

    return value;
}

ParsedFrame ProtocolParser::decode_frame() {
    ParsedFrame frame{};

    frame.command = type_;
    frame.payload.assign(payload_.begin(), payload_.begin() + length_);
    frame.type = static_cast<FrameType>(type_);

    if (frame.command > static_cast<std::uint8_t>(FrameType::Plot)) {
        return frame;
    }

    if (frame.type == FrameType::Register) {
        if (length_ < 3) {
            return frame;
        }
        frame.signal_id = payload_[0];
        frame.signal_type = static_cast<SignalType>(payload_[1]);
        const std::size_t name_length = payload_[2];
        if (length_ < 3 + name_length) {
            return frame;
        }
        frame.name = std::string(reinterpret_cast<const char*>(payload_.data() + 3), name_length);
        SignalInfo signal;
        signal.name = frame.name;
        signal.type = frame.signal_type;
        signal.signal_size = signal_size(frame.signal_type);
        signals_[frame.signal_id] = signal;
    } else if (frame.type == FrameType::Log) {
        if (length_ < 7) {
            return frame;
        }
        const std::size_t prefix_length = payload_[0];
        const std::size_t tag_length = payload_[1];
        const std::size_t message_length = payload_[2];
        if (length_ < 7 + prefix_length + tag_length + message_length) {
            return frame;
        }
        frame.timestamp = read_uint32(3);
        const std::size_t prefix_offset = 7;
        const std::size_t tag_offset = prefix_offset + prefix_length;
        const std::size_t message_offset = tag_offset + tag_length;
        frame.prefix.assign(reinterpret_cast<const char*>(payload_.data() + prefix_offset),
                            prefix_length);
        frame.tag.assign(reinterpret_cast<const char*>(payload_.data() + tag_offset), tag_length);
        frame.message.assign(reinterpret_cast<const char*>(payload_.data() + message_offset),
                             message_length);
    } else if (frame.type == FrameType::Plot) {
        if (length_ < 5) {
            return frame;
        }
        frame.signal_id = payload_[0];
        frame.timestamp = read_uint32(1);
        const auto signal = signals_.find(frame.signal_id);
        if (signal == signals_.end()) {
            return frame;
        }
        if (length_ < 5 + signal->second.signal_size) {
            return frame;
        }
        frame.name = signal->second.name;
        const std::size_t value_offset = 5;
        switch (signal->second.type) {
            case SignalType::Uint8:
                frame.value = payload_[value_offset];
                break;
            case SignalType::Uint16:
                frame.value = read_uint16(value_offset);
                break;
            case SignalType::Uint32:
                frame.value = read_uint32(value_offset);
                break;
            case SignalType::Int8:
                frame.value = static_cast<std::int8_t>(payload_[value_offset]);
                break;
            case SignalType::Int16:
                frame.value = static_cast<std::int16_t>(read_uint16(value_offset));
                break;
            case SignalType::Int32:
                frame.value = static_cast<std::int32_t>(read_uint32(value_offset));
                break;
            case SignalType::Float:
                frame.value = read_float(value_offset);
                break;
        }
    }
    return frame;
}

std::size_t ProtocolParser::signal_size(SignalType type) const {
    switch (type) {
        case SignalType::Uint8:
        case SignalType::Int8:
            return 1;

        case SignalType::Uint16:
        case SignalType::Int16:
            return 2;

        case SignalType::Uint32:
        case SignalType::Int32:
        case SignalType::Float:
            return 4;
    }

    return 0;
}

void ProtocolParser::clear() {
    reset();
    signals_.clear();
}

std::vector<ParsedFrame> ProtocolParser::push(const std::vector<std::uint8_t>& data) {
    std::vector<ParsedFrame> frames;

    for (std::uint8_t byte : data) {
        switch (state_) {
            case ParserState::Sync1:
                if (byte == protocol::sync1) {
                    state_ = ParserState::Sync2;
                }
                break;

            case ParserState::Sync2:
                if (byte == protocol::sync2) {
                    state_ = ParserState::Command;
                } else if (byte != protocol::sync1) {
                    state_ = ParserState::Sync1;
                }
                break;

            case ParserState::Command:
                type_ = byte;
                state_ = ParserState::Length;
                break;

            case ParserState::Length:
                length_ = byte;
                payload_index_ = 0;
                state_ = (byte == 0) ? ParserState::Crc : ParserState::Payload;
                break;

            case ParserState::Payload:
                payload_[payload_index_] = byte;
                payload_index_++;
                if (payload_index_ >= length_) {
                    state_ = ParserState::Crc;
                }
                break;

            case ParserState::Crc: {
                std::array<std::uint8_t, 257> crc_data{};
                crc_data[0] = type_;
                crc_data[1] = length_;
                if (length_ > 0) {
                    std::memcpy(crc_data.data() + 2, payload_.data(), length_);
                }
                const std::uint8_t expected = protocol::calculate_crc8(
                    crc_data.data(), static_cast<std::size_t>(length_) + 2);

                if (expected == byte) {
                    frames.push_back(decode_frame());
                }
                reset();
                break;
            }
        }
    }

    return frames;
}