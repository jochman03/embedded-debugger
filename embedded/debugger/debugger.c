#include <stddef.h>
#include <string.h>

#include "debugger.h"

#if DEBUGGER_ENABLED

static debugger_interface_t debugger_interface;
static bool debugger_initialized = false;

static uint8_t signal_sizes[UINT8_MAX + 1U];

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

static uint8_t signal_type_to_size(debugger_signal_type signal_type) {
    switch (signal_type) {
        case DEBUGGER_SIGNAL_TYPE_UINT8:
        case DEBUGGER_SIGNAL_TYPE_INT8:
            return 1U;
        case DEBUGGER_SIGNAL_TYPE_UINT16:
        case DEBUGGER_SIGNAL_TYPE_INT16:
            return 2U;
        case DEBUGGER_SIGNAL_TYPE_UINT32:
        case DEBUGGER_SIGNAL_TYPE_INT32:
        case DEBUGGER_SIGNAL_TYPE_FLOAT:
            return 4U;
        default:
            return 0U;
    }
}

uint8_t debugger_crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0U;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1U) ^ 0x07);
            } else {
                crc = (uint8_t)(crc << 1U);
            }
        }
    }

    return crc;
}

void debugger_plot_signal(uint8_t signal_id, const void* value) {
    if (!debugger_initialized || value == NULL) {
        return;
    }
    const uint8_t size = signal_sizes[signal_id];
    if (size == 0U) {
        return;
    }

    uint8_t frame[DEBUGGER_MAX_PLOT_FRAME_LENGTH];
    size_t index = 0U;
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
    const uint8_t crc = debugger_crc8(&frame[2U], crc_length);
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
    size_t index = 0U;
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
    parser_payload_index = 0U;
}

bool debugger_init(const debugger_interface_t* interface) {
    if (interface == NULL || interface->transmit_data == NULL ||
        interface->get_timestamp_ms == NULL) {
        return false;
    }

    debugger_interface = *interface;
    debugger_initialized = true;
    memset(signal_sizes, 0U, sizeof(signal_sizes));

    return true;
}

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

void debugger_register_signal(uint8_t signal_id, debugger_signal_type signal_type,
                              const char* signal_name) {
    if (signal_name == NULL || !debugger_initialized) {
        return;
    }
    const uint8_t size = signal_type_to_size(signal_type);
    if (size == 0U) {
        return;
    }

    const size_t name_length = strlen(signal_name);
    if (name_length > DEBUGGER_MAX_SIGNAL_NAME_LENGTH) {
        return;
    }

    signal_sizes[signal_id] = size;
    uint8_t frame[DEBUGGER_MAX_REGISTER_FRAME_LENGTH];
    size_t index = 0U;

    frame[index++] = DEBUGGER_SYNC_BYTE_1;
    frame[index++] = DEBUGGER_SYNC_BYTE_2;

    frame[index++] = DEBUGGER_FRAME_TYPE_REGISTER;
    frame[index++] = signal_id;
    frame[index++] = (uint8_t)signal_type;
    frame[index++] = (uint8_t)name_length;

    memcpy(&frame[index], signal_name, name_length);
    index += name_length;

    const size_t crc_length = index - 2U;
    const uint8_t crc = debugger_crc8(&frame[2U], crc_length);
    frame[index] = crc;
    index++;

    debugger_interface.transmit_data(frame, index);
}

void debugger_process_data(const uint8_t* data, size_t length) {
    if ((data == NULL) || (length == 0U)) {
        return;
    }
    for (size_t i = 0U; i < length; ++i) {
        const uint8_t byte = data[i];

        switch (parser_state) {
            case DEBUGGER_PARSER_SYNC_1: {
                if (byte == DEBUGGER_SYNC_BYTE_1) {
                    parser_state = DEBUGGER_PARSER_SYNC_2;
                }
                break;
            }
            case DEBUGGER_PARSER_SYNC_2: {
                if (byte == DEBUGGER_SYNC_BYTE_2) {
                    parser_state = DEBUGGER_PARSER_COMMAND;
                } else if (byte != DEBUGGER_SYNC_BYTE_1) {
                    parser_state = DEBUGGER_PARSER_SYNC_1;
                }
                break;
            }
            case DEBUGGER_PARSER_COMMAND: {
                parser_command.command_type = byte;
                parser_state = DEBUGGER_PARSER_LENGTH;
                break;
            }
            case DEBUGGER_PARSER_LENGTH: {
                if (byte > DEBUGGER_MAX_COMMAND_PAYLOAD) {
                    debugger_parser_reset();
                    break;
                }
                parser_command.length = byte;
                parser_payload_index = 0U;
                parser_state = (byte == 0U) ? DEBUGGER_PARSER_CRC : DEBUGGER_PARSER_PAYLOAD;
                break;
            }
            case DEBUGGER_PARSER_PAYLOAD: {
                parser_command.payload[parser_payload_index] = byte;
                parser_payload_index++;
                if (parser_payload_index >= parser_command.length) {
                    parser_state = DEBUGGER_PARSER_CRC;
                }
                break;
            }
            case DEBUGGER_PARSER_CRC: {
                const uint8_t expected_crc =
                    debugger_crc8(&parser_command.command_type, parser_command.length + 2);

                if (expected_crc == byte) {
                    const uint8_t next_head =
                        (uint8_t)((command_queue_head + 1U) % DEBUGGER_COMMAND_QUEUE_SIZE);

                    parser_command.argc = (parser_command.length > 0U) ? 1U : 0U;
                    for (size_t j = 0U; j < parser_command.length; ++j) {
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
            }
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
    command_queue_tail = (uint8_t)((command_queue_tail + 1U) % DEBUGGER_COMMAND_QUEUE_SIZE);
    return true;
}

const char* debugger_get_command_arg(const debugger_command_t* command, uint8_t index) {
    if (command == NULL || index >= command->argc) {
        return NULL;
    }

    size_t position = 0U;
    for (uint8_t i = 0U; i < index; ++i) {
        while (command->payload[position] != '\0') {
            position++;
        }
        position++;
    }

    return (const char*)&command->payload[position];
}

#endif /* DEBUGGER_ENABLED */
