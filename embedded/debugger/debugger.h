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

#define DEBUGGER_MAX_PREFIX_LENGTH         10
#define DEBUGGER_MAX_TAG_LENGTH            31
#define DEBUGGER_MAX_MESSAGE_LENGTH        127
#define DEBUGGER_MAX_SIGNAL_NAME_LENGTH    31
#define DEBUGGER_MAX_PLOT_FRAME_LENGTH     13
#define DEBUGGER_MAX_REGISTER_FRAME_LENGTH (DEBUGGER_MAX_SIGNAL_NAME_LENGTH + 7)
#define DEBUGGER_MAX_LOG_FRAME_LENGTH                                                              \
    (DEBUGGER_MAX_PREFIX_LENGTH + DEBUGGER_MAX_TAG_LENGTH + DEBUGGER_MAX_MESSAGE_LENGTH + 11)

#define DEBUGGER_MAX_COMMAND_PAYLOAD 32

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

typedef struct {
    void (*transmit)(uint8_t frame_type, const uint8_t* payload, uint8_t length);
    uint32_t (*get_timestamp_ms)(void);
} debugger_interface_t;

#ifndef DEBUGGER_ENABLED
#define DEBUGGER_ENABLED 1
#endif

#if DEBUGGER_ENABLED

/**
 * @brief Initializes the debugger component.
 *
 * @param interface Pointer to the communication interface.
 * @return True if initialization succeeded, otherwise false.
 *
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
bool debugger_init(const debugger_interface_t* interface);

/**
 * @brief Logs an error message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param error Error message to send
 *
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
void debugger_log_error(const char* tag, const char* error);

/**
 * @brief Logs an warning message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param warning Warning message to send
 *
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
void debugger_log_warning(const char* tag, const char* warning);

/**
 * @brief Logs info message with a source tag.
 *
 * @param tag Short text containing source file name
 * @param info Info message to send
 *
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
void debugger_log_info(const char* tag, const char* info);

/**
 * @brief Logs reset message with a optional reason.
 *
 * @param tag Short text containing source file name
 * @param reason Short text containing potential reason
 *
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
void debugger_log_reset(const char* tag, const char* reason);

/**
 * @brief Registers a signal and sends its metadata to the host.
 *
 * The signal type determines the number of bytes read by
 * debugger_plot_signal().
 *
 * @param signal_id Application-defined signal identifier.
 * @param signal_type Type of the signal value.
 * @param signal_name Human-readable signal name.
 *
 * @note Register the signal before sending its value.
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
void debugger_register_signal(uint8_t signal_id, debugger_signal_type signal_type,
                              const char* signal_name);
/**
 * @brief Sends the current value of a registered signal.
 *
 * @param signal_id Identifier previously passed to debugger_register_signal().
 * @param value Pointer to the signal value.
 *
 * @note The pointed object must match the registered signal type.
 * @note Available only when DEBUGGER_ENABLED is set to 1.
 */
void debugger_plot_signal(uint8_t signal_id, const void* value);

#else

#define debugger_init(interface)                 (true)
#define debugger_log_error(tag, error)           ((void)0)
#define debugger_log_warning(tag, warning)       ((void)0)
#define debugger_log_info(tag, info)             ((void)0)
#define debugger_log_reset(tag, reason)          ((void)0)
#define debugger_register_signal(id, type, name) ((void)0)
#define debugger_plot_signal(id, value)          ((void)0)
#endif /* DEBUGGER_ENABLED */

#endif /* DEBUGGER_H */
