#include <stddef.h>
#include <string.h>

#include "frame.h"

typedef enum frame_parser_state {
    FRAME_PARSER_SYNC_1,
    FRAME_PARSER_SYNC_2,
    FRAME_PARSER_COMMAND,
    FRAME_PARSER_LENGTH,
    FRAME_PARSER_PAYLOAD,
    FRAME_PARSER_CRC
} frame_parser_state;

uint8_t frame_crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0U;

    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];

        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1U) ^ 0x07);

            } else {
                crc = (uint8_t)(crc << 1U);
            }
        }
    }
    return crc;
}

static void frame_parser_reset(frame_parser_t* parser) {
    parser->parser_state = FRAME_PARSER_SYNC_1;
    parser->parser_payload_index = 0U;
}

bool frame_parser_init(frame_parser_t* parser, frame_t* command_queue, uint8_t command_queue_size) {
    if ((parser == NULL) || (command_queue == NULL) || (command_queue_size == 0U)) {
        return false;
    }
    parser->command_queue = command_queue;
    parser->command_queue_size = command_queue_size;
    parser->command_queue_head = 0U;
    parser->command_queue_tail = 0U;
    memset(parser->parser_command.payload, 0U, sizeof(parser->parser_command.payload));
    frame_parser_reset(parser);

    return true;
}

size_t frame_encode(uint8_t* data, uint8_t command_type, const uint8_t* payload, uint8_t length) {
    if ((data == NULL) || (length > FRAME_MAX_COMMAND_PAYLOAD) ||
        ((payload == NULL) && (length > 0U))) {
        return 0U;
    }

    size_t index = 0U;
    data[index++] = FRAME_SYNC_BYTE_1;
    data[index++] = FRAME_SYNC_BYTE_2;
    data[index++] = command_type;
    data[index++] = length;
    if (length > 0U) {
        memcpy(&data[index], payload, length);
        index += length;
    }
    data[index] = frame_crc8(&data[2U], (size_t)length + 2U);
    index++;

    return index;
}

void frame_decode(frame_parser_t* parser, const uint8_t* data, size_t length) {
    if ((parser == NULL) || (parser->command_queue == NULL) || (data == NULL) || (length == 0U)) {
        return;
    }
    for (size_t i = 0; i < length; ++i) {
        const uint8_t byte = data[i];

        switch (parser->parser_state) {
            case FRAME_PARSER_SYNC_1: {
                if (byte == FRAME_SYNC_BYTE_1) {
                    parser->parser_state = FRAME_PARSER_SYNC_2;
                }
                break;
            }
            case FRAME_PARSER_SYNC_2: {
                if (byte == FRAME_SYNC_BYTE_2) {
                    parser->parser_state = FRAME_PARSER_COMMAND;
                } else if (byte != FRAME_SYNC_BYTE_1) {
                    parser->parser_state = FRAME_PARSER_SYNC_1;
                }
                break;
            }
            case FRAME_PARSER_COMMAND: {
                parser->parser_command.command_type = byte;
                parser->parser_state = FRAME_PARSER_LENGTH;
                break;
            }
            case FRAME_PARSER_LENGTH: {
                if (byte > FRAME_MAX_COMMAND_PAYLOAD) {
                    frame_parser_reset(parser);
                    break;
                }
                parser->parser_command.length = byte;
                parser->parser_payload_index = 0U;
                parser->parser_state = (byte == 0U) ? FRAME_PARSER_CRC : FRAME_PARSER_PAYLOAD;
                break;
            }
            case FRAME_PARSER_PAYLOAD: {
                parser->parser_command.payload[parser->parser_payload_index] = byte;
                parser->parser_payload_index++;
                if (parser->parser_payload_index >= parser->parser_command.length) {
                    parser->parser_state = FRAME_PARSER_CRC;
                }
                break;
            }
            case FRAME_PARSER_CRC: {
                const uint8_t expected_crc = frame_crc8(&parser->parser_command.command_type,
                                                        parser->parser_command.length + 2U);

                if (expected_crc == byte) {
                    const uint8_t next_head =
                        (uint8_t)((parser->command_queue_head + 1U) % parser->command_queue_size);
                    if (next_head != parser->command_queue_tail) {
                        parser->command_queue[parser->command_queue_head] = parser->parser_command;
                        parser->command_queue_head = next_head;
                    }
                }
                frame_parser_reset(parser);
                break;
            }
            default:
                continue;
        }
    }
}

bool frame_parser_get(frame_parser_t* parser, frame_t* frame) {
    if ((parser == NULL) || (frame == NULL) || (parser->command_queue == NULL)) {
        return false;
    }
    if (parser->command_queue_head == parser->command_queue_tail) {
        return false;
    }

    *frame = parser->command_queue[parser->command_queue_tail];
    parser->command_queue_tail =
        (uint8_t)((parser->command_queue_tail + 1U) % parser->command_queue_size);
    return true;
}