#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "debugger.h"
#include "ntc.h"

#include "platform_uart.h"
#include "platform_adc.h"
#include "platform_led.h"
#include "platform.h"

#if DEBUGGER_ENABLED
static const char tag[] = "APP";
#endif /* DEBUGGER_ENABLED */

static int8_t counter = -100;

static uint16_t adc_temperature = 0;
static uint32_t counter_delay = 0;
static debugger_command_t current_command;

typedef enum app_command { APP_COMMAND_REQUEST_REGISTERS = 1, APP_COMMAND_USER } app_command;

typedef enum signal { SIGNAL_COUNTER8, SIGNAL_TEMPERATURE, SIGNAL_LIGHT } signal;

static void send_registers(void) {
    debugger_register_signal(SIGNAL_COUNTER8, DEBUGGER_SIGNAL_TYPE_INT8, "Counter8");
    debugger_register_signal(SIGNAL_TEMPERATURE, DEBUGGER_SIGNAL_TYPE_FLOAT, "Temperature");
}

void app_init(void) {
    platform_adc_init();

#if DEBUGGER_ENABLED
    const debugger_interface_t interface = {
        .transmit_data = platform_uart_transmit,
        .get_timestamp_ms = platform_get_timestamp_ms,
    };
    (void)debugger_init(&interface);
    platform_uart_init(debugger_process_data);

    debugger_log_reset(tag, "App start");

#endif /* DEBUGGER_ENABLED */
}

void app_update(void) {
    if (platform_adc_get(&adc_temperature)) {
        const float temperature = ntc_calculate_temperature(adc_temperature);
        if (!isnan(temperature)) {
            debugger_plot_signal(SIGNAL_TEMPERATURE, &temperature);
        }
    }
    if (counter_delay == 1000) {
        debugger_log_warning(tag, "This is a test warning");
        counter_delay++;
    } else if (counter_delay == 2000) {
        debugger_log_error(tag, "This is a test error");
        counter_delay++;
    } else if (counter_delay == 3000) {
        debugger_log_info(tag, "This is a test message");

        debugger_plot_signal(SIGNAL_COUNTER8, &counter);

        counter++;
        counter_delay = 0;
    } else {
        counter_delay++;
    }
    if (debugger_get_command(&current_command)) {
        const app_command command = (app_command)current_command.command_type;
        switch (command) {
            case APP_COMMAND_REQUEST_REGISTERS:
                debugger_log_info(tag, "Registers requested");
                send_registers();
                break;
            case APP_COMMAND_USER: {
                const char* cmd = debugger_get_command_arg(&current_command, 0);
                if (cmd != NULL && strcmp(cmd, "set_led") == 0) {
                    const char* value = debugger_get_command_arg(&current_command, 1);
                    if (value != NULL) {
                        if (strcmp(value, "0") == 0) {
                            platform_led_set(0);
                        } else if (strcmp(value, "1") == 0) {
                            platform_led_set(1);
                        }
                    }
                }
                break;
            }

            default: {
                char buff[32];
                snprintf(buff, sizeof(buff), "Invalid command with id: %d",
                         current_command.command_type);
                debugger_log_warning(tag, buff);
                break;
            }
        }
    }

    platform_delay(1);
}
