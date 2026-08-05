/*
 *  debugger.h
 *
 *  Created on: 21 Jul 2026
 *  Author: jochman03
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DEBUGGER_SYNC_BYTE_1 0xA0
#define DEBUGGER_SYNC_BYTE_2 0x0A

#define DEBUGGER_MAX_PREFIX_LENGTH         10
#define DEBUGGER_MAX_TAG_LENGTH            31
#define DEBUGGER_MAX_MESSAGE_LENGTH        127
#define DEBUGGER_MAX_SIGNAL_NAME_LENGTH    31
#define DEBUGGER_MAX_PLOT_FRAME_LENGTH     13
#define DEBUGGER_MAX_REGISTER_FRAME_LENGTH (DEBUGGER_MAX_SIGNAL_NAME_LENGTH + 7)
#define DEBUGGER_MAX_LOG_FRAME_LENGTH                                                              \
    (DEBUGGER_MAX_PREFIX_LENGTH + DEBUGGER_MAX_TAG_LENGTH + DEBUGGER_MAX_MESSAGE_LENGTH + 11)

#define DEBUGGER_MAX_COMMAND_PAYLOAD 32
#define DEBUGGER_COMMAND_QUEUE_SIZE  5

typedef enum debugger_frame_type {
    DEBUGGER_FRAME_TYPE_REGISTER = 0,
    DEBUGGER_FRAME_TYPE_LOG,
    DEBUGGER_FRAME_TYPE_PLOT,
} debugger_frame_type;

typedef enum debugger_signal_type {
    DEBUGGER_SIGNAL_TYPE_UINT8,
    DEBUGGER_SIGNAL_TYPE_UINT16,
    DEBUGGER_SIGNAL_TYPE_UINT32,
    DEBUGGER_SIGNAL_TYPE_INT8,
    DEBUGGER_SIGNAL_TYPE_INT16,
    DEBUGGER_SIGNAL_TYPE_INT32,
    DEBUGGER_SIGNAL_TYPE_FLOAT
} debugger_signal_type;

typedef struct debugger_command_t {
    uint8_t command_type;
    uint8_t length;
    uint8_t payload[DEBUGGER_MAX_COMMAND_PAYLOAD];
} debugger_command_t;

typedef struct {
    void (*transmit_data)(const uint8_t* data, size_t length);
    uint32_t (*get_timestamp_ms)(void);
} debugger_interface_t;

/**
 * @brief Initializes the debugger component.
 *
 * @param interface Pointer to the communication interface.
 * @return True if initialization succeeded, otherwise false.
 */
bool debugger_init(const debugger_interface_t* interface);

#define DEBUGGER_LOGGING_ENABLED 1
#define DEBUGGER_ENABLED         1

#if DEBUGGER_ENABLED && DEBUGGER_LOGGING_ENABLED

/**
 * @brief Logs an error message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param error Error message to send
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
void debugger_log_error(const char* tag, const char* error);

/**
 * @brief Logs an warning message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param warning Warning message to send
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
void debugger_log_warning(const char* tag, const char* warning);

/**
 * @brief Logs info message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param info Info message to send
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
void debugger_log_info(const char* tag, const char* info);

/**
 * @brief Logs reset message with a optional reason.
 *
 * @param tag Short text containing source file name
 * @param reason Short text containing potential reason
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
void debugger_log_reset(const char* tag, const char* reason);

void debugger_register_signal(const uint8_t signal_id, const debugger_signal_type signal_type,
                              const char* signal_name);
void debugger_plot_signal_uint8(const uint8_t signal_id, const uint8_t value);
void debugger_plot_signal_uint16(const uint8_t signal_id, const uint16_t value);
void debugger_plot_signal_uint32(const uint8_t signal_id, const uint32_t value);

void debugger_plot_signal_int8(const uint8_t signal_id, const int8_t value);
void debugger_plot_signal_int16(const uint8_t signal_id, const int16_t value);
void debugger_plot_signal_int32(const uint8_t signal_id, const int32_t value);

void debugger_plot_signal_float(const uint8_t signal_id, const float value);

void debugger_process_data(const uint8_t* data, size_t length);
bool debugger_get_command(debugger_command_t* command);

#else

/**
 * @brief Logs an error message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param error Error message to send
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
#define debugger_log_error(tag, error)     ((void)0)

/**
 * @brief Logs an warning message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param warning Warning message to send
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
#define debugger_log_warning(tag, warning) ((void)0)

/**
 * @brief Logs info message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param info Info message to send
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
#define debugger_log_info(tag, info)       ((void)0)

/**
 * @brief Logs reset message with a optional reason.
 *
 * @param tag Short text containing source file name
 * @param reason Short text containing potential reason
 *
 * @note Available only when DEBUGGER_LOGGING_ENABLED is set to 1.
 */
#define debugger_log_reset(tag, reason)    ((void)0)

#define debugger_plot_uint8(signal_id, value) ((void)0)

#define debugger_plot_uint16(signal_id, value) ((void)0)

#define debugger_plot_uint32(signal_id, value) ((void)0)

#define debugger_plot_int8(signal_id, value) ((void)0)

#define debugger_plot_int16(signal_id, value) ((void)0)

#define debugger_plot_int32(signal_id, value) ((void)0)

#define debugger_plot_float(signal_id, value) ((void)0)

#endif

#endif /* DEBUGGER_H */
