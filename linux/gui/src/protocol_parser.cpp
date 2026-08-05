#include <cstring>

#include "protocol.hpp"
#include "protocol_parser.hpp"

std::size_t ProtocolParser::find_sync() const {
    for (std::size_t i = 0; i + 1 < buffer_.size(); ++i) {
        if (buffer_[i] == protocol::sync1 && buffer_[i + 1] == protocol::sync2) {
            return i;
        }
    }

    return buffer_.size();
}

bool ProtocolParser::check_crc(std::size_t frame_size) const {
    if (frame_size < 4 || buffer_.size() < frame_size) {
        return false;
    }

    const std::size_t data_size = frame_size - 3;

    const std::uint8_t calculated_crc = protocol::calculate_crc8(buffer_.data() + 2, data_size);

    const std::uint8_t received_crc = buffer_[frame_size - 1];

    return calculated_crc == received_crc;
}

std::uint16_t ProtocolParser::read_uint16(std::size_t offset) const {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(buffer_[offset]) |
                                      (static_cast<std::uint16_t>(buffer_[offset + 1]) << 8));
}

std::uint32_t ProtocolParser::read_uint32(std::size_t offset) const {
    return static_cast<std::uint32_t>(buffer_[offset]) |
           (static_cast<std::uint32_t>(buffer_[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer_[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer_[offset + 3]) << 24);
}

float ProtocolParser::read_float(std::size_t offset) const {
    const std::uint32_t raw_value = read_uint32(offset);

    float value = 0.0f;

    std::memcpy(&value, &raw_value, sizeof(value));

    return value;
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

int ProtocolParser::expected_frame_size() const {
    if (buffer_.size() < 3) {
        return 0;
    }

    const FrameType frame_type = static_cast<FrameType>(buffer_[2]);

    if (frame_type == FrameType::Register) {
        if (buffer_.size() < 6) {
            return 0;
        }

        const SignalType signal_type = static_cast<SignalType>(buffer_[4]);

        if (signal_size(signal_type) == 0) {
            return -1;
        }

        const std::size_t name_length = buffer_[5];

        return static_cast<int>(7 + name_length);
    }

    if (frame_type == FrameType::Log) {
        if (buffer_.size() < 6) {
            return 0;
        }

        const std::size_t prefix_length = buffer_[3];
        const std::size_t tag_length = buffer_[4];
        const std::size_t message_length = buffer_[5];

        return static_cast<int>(11 + prefix_length + tag_length + message_length);
    }
    if (frame_type == FrameType::Plot) {
        if (buffer_.size() < 4) {
            return 0;
        }

        const std::uint8_t signal_id = buffer_[3];

        const auto signal = signals_.find(signal_id);

        if (signal == signals_.end()) {
            return -1;
        }

        return static_cast<int>(9 + signal->second.signal_size);
    }
    return -1;
}

ParsedFrame ProtocolParser::decode_frame(std::size_t frame_size) {
    ParsedFrame frame{};

    frame.type = static_cast<FrameType>(buffer_[2]);

    if (frame.type == FrameType::Register) {
        frame.signal_id = buffer_[3];
        frame.signal_type = static_cast<SignalType>(buffer_[4]);

        const std::size_t name_length = buffer_[5];
        const std::size_t name_offset = 6;

        frame.name =
            std::string(reinterpret_cast<const char*>(buffer_.data() + name_offset), name_length);
        SignalInfo signal;

        signal.name = frame.name;
        signal.type = frame.signal_type;
        signal.signal_size = signal_size(frame.signal_type);

        signals_[frame.signal_id] = signal;
    } else if (frame.type == FrameType::Log) {
        const std::size_t prefix_length = buffer_[3];
        const std::size_t tag_length = buffer_[4];
        const std::size_t message_length = buffer_[5];

        frame.timestamp = read_uint32(6);

        const std::size_t prefix_offset = 10;
        const std::size_t tag_offset = prefix_offset + prefix_length;
        const std::size_t message_offset = tag_offset + tag_length;

        frame.prefix.assign(reinterpret_cast<const char*>(buffer_.data() + prefix_offset),
                            prefix_length);
        frame.tag.assign(reinterpret_cast<const char*>(buffer_.data() + tag_offset), tag_length);
        frame.message.assign(reinterpret_cast<const char*>(buffer_.data() + message_offset),
                             message_length);
    } else if (frame.type == FrameType::Plot) {
        frame.signal_id = buffer_[3];
        frame.timestamp = read_uint32(4);

        const auto signal = signals_.find(frame.signal_id);

        if (signal == signals_.end()) {
            return frame;
        }

        frame.name = signal->second.name;

        const std::size_t value_offset = 8;

        switch (signal->second.type) {
            case SignalType::Uint8:
                frame.value = buffer_[value_offset];
                break;
            case SignalType::Uint16:
                frame.value = read_uint16(value_offset);
                break;
            case SignalType::Uint32:
                frame.value = read_uint32(value_offset);
                break;
            case SignalType::Int8:
                frame.value = static_cast<std::int8_t>(buffer_[value_offset]);
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

std::vector<ParsedFrame> ProtocolParser::push(const std::vector<std::uint8_t>& data) {
    std::vector<ParsedFrame> frames;

    buffer_.insert(buffer_.end(), data.begin(), data.end());

    while (true) {
        if (buffer_.size() < 2) {
            break;
        }

        const std::size_t sync_position = find_sync();

        if (sync_position == buffer_.size()) {
            const bool keep_last_a0 = !buffer_.empty() && buffer_.back() == protocol::sync1;

            buffer_.clear();

            if (keep_last_a0) {
                buffer_.push_back(protocol::sync1);
            }

            break;
        }

        if (sync_position > 0) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + sync_position);
        }

        const int expected_size = expected_frame_size();

        if (expected_size == 0) {
            break;
        }

        if (expected_size < 0) {
            buffer_.erase(buffer_.begin());
            continue;
        }

        const std::size_t frame_size = static_cast<std::size_t>(expected_size);

        if (buffer_.size() < frame_size) {
            break;
        }

        const std::uint8_t calculated_crc =
            protocol::calculate_crc8(buffer_.data() + 2, frame_size - 3);

        if (!check_crc(frame_size)) {
            buffer_.erase(buffer_.begin());
            continue;
        }

        frames.push_back(decode_frame(frame_size));

        buffer_.erase(buffer_.begin(), buffer_.begin() + frame_size);
    }

    return frames;
}