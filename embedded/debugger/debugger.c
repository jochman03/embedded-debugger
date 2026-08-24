#include <stddef.h>
#include <string.h>

#include "debugger.h"

#if DEBUGGER_ENABLED

static debugger_interface_t debugger_interface;
static bool debugger_initialized = false;

static uint8_t signal_sizes[UINT8_MAX + 1U];
static uint8_t log_payload[3U + 4U + DEBUGGER_MAX_PREFIX_LENGTH + DEBUGGER_MAX_TAG_LENGTH +
                           DEBUGGER_MAX_MESSAGE_LENGTH];

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

void debugger_plot_signal(uint8_t signal_id, const void* value) {
    if (!debugger_initialized || value == NULL) {
        return;
    }
    const uint8_t size = signal_sizes[signal_id];
    if (size == 0U) {
        return;
    }

    uint8_t payload[9U];
    uint8_t payload_length = 0U;
    const uint32_t timestamp = debugger_interface.get_timestamp_ms();

    payload[payload_length++] = signal_id;
    memcpy(&payload[payload_length], &timestamp, sizeof(timestamp));
    payload_length = (uint8_t)(payload_length + sizeof(timestamp));
    memcpy(&payload[payload_length], value, size);
    payload_length = (uint8_t)(payload_length + size);

    debugger_interface.transmit((uint8_t)DEBUGGER_FRAME_TYPE_PLOT, payload, payload_length);
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
    uint8_t payload_length = 0U;
    const uint32_t timestamp = debugger_interface.get_timestamp_ms();

    log_payload[payload_length++] = (uint8_t)prefix_length;
    log_payload[payload_length++] = (uint8_t)tag_length;
    log_payload[payload_length++] = (uint8_t)message_length;
    memcpy(&log_payload[payload_length], &timestamp, sizeof(timestamp));
    payload_length = (uint8_t)(payload_length + sizeof(timestamp));
    memcpy(&log_payload[payload_length], prefix, prefix_length);
    payload_length = (uint8_t)(payload_length + prefix_length);
    memcpy(&log_payload[payload_length], tag, tag_length);
    payload_length = (uint8_t)(payload_length + tag_length);
    memcpy(&log_payload[payload_length], message, message_length);
    payload_length = (uint8_t)(payload_length + message_length);

    debugger_interface.transmit((uint8_t)DEBUGGER_FRAME_TYPE_LOG, log_payload, payload_length);
}

bool debugger_init(const debugger_interface_t* interface) {
    if (interface == NULL || interface->transmit == NULL || interface->get_timestamp_ms == NULL) {
        return false;
    }

    debugger_interface = *interface;
    debugger_initialized = true;
    memset(signal_sizes, 0U, sizeof(signal_sizes));

    return true;
}

void debugger_log_error(const char* tag, const char* error) {
    if (!debugger_initialized || debugger_interface.transmit == NULL || tag == NULL ||
        error == NULL) {
        return;
    }
    debugger_log("[ERROR]", tag, error);
}

void debugger_log_warning(const char* tag, const char* warning) {
    if (!debugger_initialized || debugger_interface.transmit == NULL || tag == NULL ||
        warning == NULL) {
        return;
    }
    debugger_log("[WARNING]", tag, warning);
}

void debugger_log_info(const char* tag, const char* info) {
    if (!debugger_initialized || debugger_interface.transmit == NULL || tag == NULL ||
        info == NULL) {
        return;
    }
    debugger_log("[INFO]", tag, info);
}

void debugger_log_reset(const char* tag, const char* reason) {
    if (!debugger_initialized || debugger_interface.transmit == NULL || reason == NULL) {
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
    uint8_t payload[3U + DEBUGGER_MAX_SIGNAL_NAME_LENGTH];
    uint8_t payload_length = 0U;

    payload[payload_length++] = signal_id;
    payload[payload_length++] = (uint8_t)signal_type;
    payload[payload_length++] = (uint8_t)name_length;
    memcpy(&payload[payload_length], signal_name, name_length);
    payload_length = (uint8_t)(payload_length + name_length);

    debugger_interface.transmit((uint8_t)DEBUGGER_FRAME_TYPE_REGISTER, payload, payload_length);
}

#endif /* DEBUGGER_ENABLED */
