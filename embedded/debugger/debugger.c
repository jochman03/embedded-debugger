#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debugger.h"
#include "debugger.h"

static debugger_interface_t debugger_interface;
static bool debugger_initialized = false;

typedef enum debugger_parser_state {
    DEBUGGER_PARSER_SYNC_1,
    DEBUGGER_PARSER_SYNC_2,
    DEBUGGER_PARSER_COMMAND,
    DEBUGGER_PARSER_LENGTH,
    DEBUGGER_PARSER_PAYLOAD,
    DEBUGGER_PARSER_CRC
} debugger_parser_state;
static debugger_parser_state parser_state = DEBUGGER_PARSER_SYNC_1;
static debugger_command_t parser_command;
static uint8_t parser_payload_index;

static debugger_command_t command_queue[DEBUGGER_COMMAND_QUEUE_SIZE];
static volatile uint8_t command_queue_head;
static volatile uint8_t command_queue_tail;

uint8_t debugger_crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1) ^ 0x07);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    return crc;
}

static void plot_signal(const uint8_t signal_id, const void* value, const size_t size) {
    if ((value == NULL) || (size == 0U) || (size > sizeof(uint32_t))) {
        return;
    }
    uint8_t frame[DEBUGGER_MAX_PLOT_FRAME_LENGTH];
    size_t index = 0;
    const uint32_t timestamp = debugger_interface.get_timestamp_ms();

    frame[index++] = DEBUGGER_SYNC_BYTE_1;
    frame[index++] = DEBUGGER_SYNC_BYTE_2;

    frame[index++] = DEBUGGER_FRAME_TYPE_PLOT;
    frame[index++] = signal_id;

    memcpy(&frame[index], &timestamp, sizeof(timestamp));
    index += sizeof(timestamp);

    memcpy(&frame[index], value, size);
    index += size;

    const size_t crc_length = index - 2U;
    const uint8_t crc = debugger_crc8(&frame[2], crc_length);
    frame[index] = crc;
    index++;

    debugger_interface.transmit_data(frame, index);
}

static void debugger_log(const char* prefix, const char* tag, const char* message) {
    if ((prefix == NULL) || (tag == NULL) || (message == NULL)) {
        return;
    }

    const size_t prefix_length = strlen(prefix);
    const size_t tag_length = strlen(tag);
    const size_t message_length = strlen(message);

    if ((prefix_length > DEBUGGER_MAX_PREFIX_LENGTH) || (tag_length > DEBUGGER_MAX_TAG_LENGTH) ||
        (message_length > DEBUGGER_MAX_MESSAGE_LENGTH)) {
        return;
    }

    uint8_t frame[DEBUGGER_MAX_LOG_FRAME_LENGTH];
    size_t index = 0;
    const uint32_t timestamp = debugger_interface.get_timestamp_ms();

    frame[index++] = DEBUGGER_SYNC_BYTE_1;
    frame[index++] = DEBUGGER_SYNC_BYTE_2;

    frame[index++] = DEBUGGER_FRAME_TYPE_LOG;
    frame[index++] = (uint8_t)prefix_length;
    frame[index++] = (uint8_t)tag_length;
    frame[index++] = (uint8_t)message_length;

    memcpy(&frame[index], &timestamp, sizeof(timestamp));
    index += sizeof(timestamp);

    memcpy(&frame[index], prefix, prefix_length);
    index += prefix_length;

    memcpy(&frame[index], tag, tag_length);
    index += tag_length;

    memcpy(&frame[index], message, message_length);
    index += message_length;

    const size_t crc_length = index - 2U;
    const uint8_t crc = debugger_crc8(&frame[2], crc_length);
    frame[index] = crc;
    index++;

    debugger_interface.transmit_data(frame, index);
}

static void debugger_parser_reset(void) {
    parser_state = DEBUGGER_PARSER_SYNC_1;
    parser_payload_index = 0;
}

bool debugger_init(const debugger_interface_t* interface) {
    if (interface == NULL || interface->transmit_data == NULL ||
        interface->get_timestamp_ms == NULL) {
        return false;
    }

    debugger_interface = *interface;
    debugger_initialized = true;
    return true;
}

#if DEBUGGER_LOGGING_ENABLED

void debugger_log_error(const char* tag, const char* error) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL || tag == NULL ||
        error == NULL) {
        return;
    }
    debugger_log("[ERROR]", tag, error);
}

void debugger_log_warning(const char* tag, const char* warning) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL || tag == NULL ||
        warning == NULL) {
        return;
    }
    debugger_log("[WARNING]", tag, warning);
}

void debugger_log_info(const char* tag, const char* info) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL || tag == NULL ||
        info == NULL) {
        return;
    }
    debugger_log("[INFO]", tag, info);
}

void debugger_log_reset(const char* tag, const char* reason) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL || reason == NULL) {
        return;
    }
    debugger_log("[RESET]", tag, reason);
}

void debugger_register_signal(const uint8_t signal_id, const debugger_signal_type signal_type,
                              const char* signal_name) {
    if (signal_name == NULL) {
        return;
    }
    size_t name_length = strlen(signal_name);
    if (name_length > DEBUGGER_MAX_SIGNAL_NAME_LENGTH) {
        return;
    }

    uint8_t frame[DEBUGGER_MAX_REGISTER_FRAME_LENGTH];
    size_t index = 0;

    frame[index++] = DEBUGGER_SYNC_BYTE_1;
    frame[index++] = DEBUGGER_SYNC_BYTE_2;

    frame[index++] = DEBUGGER_FRAME_TYPE_REGISTER;
    frame[index++] = signal_id;
    frame[index++] = (uint8_t)signal_type;
    frame[index++] = (uint8_t)name_length;

    memcpy(&frame[index], signal_name, name_length);
    index += name_length;

    const size_t crc_length = index - 2U;
    const uint8_t crc = debugger_crc8(&frame[2], crc_length);
    frame[index] = crc;
    index++;

    debugger_interface.transmit_data(frame, index);
}

void debugger_plot_signal_uint8(const uint8_t signal_id, const uint8_t value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 1);
}

void debugger_plot_signal_uint16(const uint8_t signal_id, const uint16_t value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 2);
}

void debugger_plot_signal_uint32(const uint8_t signal_id, const uint32_t value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 4);
}

void debugger_plot_signal_int8(const uint8_t signal_id, const int8_t value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 1);
}

void debugger_plot_signal_int16(const uint8_t signal_id, const int16_t value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 2);
}

void debugger_plot_signal_int32(const uint8_t signal_id, const int32_t value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 4);
}

void debugger_plot_signal_float(const uint8_t signal_id, const float value) {
    if (!debugger_initialized || debugger_interface.transmit_data == NULL) {
        return;
    }
    plot_signal(signal_id, &value, 4);
}

void debugger_process_data(const uint8_t* data, size_t length) {
    if ((data == NULL) || (length == 0)) {
        return;
    }
    for (size_t i = 0; i < length; ++i) {
        const uint8_t byte = data[i];

        switch (parser_state) {
            case DEBUGGER_PARSER_SYNC_1:
                if (byte == DEBUGGER_SYNC_BYTE_1) {
                    parser_state = DEBUGGER_PARSER_SYNC_2;
                }
                break;
            case DEBUGGER_PARSER_SYNC_2:
                if (byte == DEBUGGER_SYNC_BYTE_2) {
                    parser_state = DEBUGGER_PARSER_COMMAND;
                } else if (byte != DEBUGGER_SYNC_BYTE_1) {
                    parser_state = DEBUGGER_PARSER_SYNC_1;
                }
                break;
            case DEBUGGER_PARSER_COMMAND:
                parser_command.command_type = byte;
                parser_state = DEBUGGER_PARSER_LENGTH;
                break;
            case DEBUGGER_PARSER_LENGTH:
                if (byte > DEBUGGER_MAX_COMMAND_PAYLOAD) {
                    debugger_parser_reset();
                    break;
                }
                parser_command.length = byte;
                parser_payload_index = 0;
                parser_state = (byte == 0) ? DEBUGGER_PARSER_CRC : DEBUGGER_PARSER_PAYLOAD;
                break;
            case DEBUGGER_PARSER_PAYLOAD:
                parser_command.payload[parser_payload_index] = byte;
                parser_payload_index++;
                if (parser_payload_index >= parser_command.length) {
                    parser_state = DEBUGGER_PARSER_CRC;
                }
                break;
            case DEBUGGER_PARSER_CRC:
                uint8_t expected_crc =
                    debugger_crc8(&parser_command.command_type, parser_command.length + 2);

                if (expected_crc == byte) {
                    const uint8_t next_head =
                        (uint8_t)((command_queue_head + 1U) % DEBUGGER_COMMAND_QUEUE_SIZE);

                    parser_command.argc = (parser_command.length > 0) ? 1 : 0;
                    for (size_t j = 0; j < parser_command.length; ++j) {
                        if (parser_command.payload[j] == ' ') {
                            parser_command.payload[j] = '\0';
                            parser_command.argc++;
                        }
                    }
                    parser_command.payload[parser_command.length] = '\0';

                    if (next_head != command_queue_tail) {
                        command_queue[command_queue_head] = parser_command;

                        command_queue_head = next_head;
                    }
                }
                debugger_parser_reset();
                break;
            default:
                continue;
        }
    }
}

bool debugger_get_command(debugger_command_t* command) {
    if (command == NULL) {
        return false;
    }
    if (command_queue_head == command_queue_tail) {
        return false;
    }
    *command = command_queue[command_queue_tail];
    command_queue_tail = (uint8_t)((command_queue_tail + 1) % DEBUGGER_COMMAND_QUEUE_SIZE);
    return true;
}

const char* debugger_get_command_arg(debugger_command_t* command, uint8_t index) {
    if (command == NULL || index > command->argc) {
        return NULL;
    }

    size_t position = 0;
    for (uint8_t i = 0; i < index; ++i) {
        while (command->payload[position] != '\0') {
            position++;
        }
        position++;
    }

    return (const char*)&command->payload[position];
}

#endif