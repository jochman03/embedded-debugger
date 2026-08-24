#include "app_debug.h"

#include "frame.h"
#include "debugger.h"

#include "platform_uart.h"
#include "platform_led.h"
#include "platform.h"

static const char tag[] = "APP_DEBUG";
static frame_t command_queue[FRAME_COMMAND_QUEUE_SIZE];
static frame_parser_t parser;

static void app_debug_transmit(uint8_t frame_type, const uint8_t* payload, uint8_t length);
static void app_debug_uart_rx(const uint8_t* data, size_t length);
static void app_debug_handle_command(const frame_t* frame);
static void send_registers(void);

void app_debug_init(void) {
    platform_uart_init(app_debug_uart_rx);

    (void)frame_parser_init(&parser, command_queue, FRAME_COMMAND_QUEUE_SIZE);

    const debugger_interface_t interface = {.transmit = app_debug_transmit,
                                            .get_timestamp_ms = platform_get_timestamp_ms};
    (void)debugger_init(&interface);

    debugger_log_reset(tag, "App start");
    send_registers();
}

void app_debug_poll(void) {
    frame_t frame;
    while (frame_parser_get(&parser, &frame)) {
        app_debug_handle_command(&frame);
    }
}

static void app_debug_transmit(uint8_t frame_type, const uint8_t* payload, uint8_t length) {
    uint8_t buffer[5U + DEBUGGER_MAX_LOG_FRAME_LENGTH];
    const size_t encoded = frame_encode(buffer, frame_type, payload, length);
    if (encoded > 0U) {
        platform_uart_transmit(buffer, encoded);
    }
}

static void app_debug_uart_rx(const uint8_t* data, size_t length) {
    frame_decode(&parser, data, length);
}

static void app_debug_handle_command(const frame_t* frame) {
    if (frame == NULL) {
        return;
    }

    switch ((app_command)frame->command_type) {
        case APP_COMMAND_REQUEST_REGISTERS:
            send_registers();
            break;
        case APP_COMMAND_SET_LED:
            if (frame->length == 1U) {
                platform_led_set(frame->payload[0] != 0U);
            }
            break;
        default:
            break;
    }
}

static void send_registers(void) {
    debugger_register_signal(SIGNAL_COUNTER8, DEBUGGER_SIGNAL_TYPE_INT8, "Counter8");
    debugger_register_signal(SIGNAL_TEMPERATURE, DEBUGGER_SIGNAL_TYPE_FLOAT, "Temperature");
}
