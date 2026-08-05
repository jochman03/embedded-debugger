#include <math.h>
#include <stdio.h>

#include "app.h"
#include "debugger.h"
#include "ntc.h"

#include "bsp_uart.h"
#include "bsp_adc.h"
#include "bsp.h"

static char tag[] = "APP";

int8_t counter = -100;
uint16_t counter16 = 0;

uint16_t adc_temperature = 0;
uint32_t counter_delay = 0;
bool transmit_counter = false;
static debugger_command_t current_command;

typedef enum app_command { APP_COMMAND_REQUEST_REGISTERS = 1 } app_command;

typedef enum signal { SIGNAL_COUNTER8, SIGNAL_COUNTER16, SIGNAL_TEMPERATURE, SIGNAL_LIGHT } signal;

static void send_registers() {
    debugger_register_signal(SIGNAL_COUNTER8, DEBUGGER_SIGNAL_TYPE_INT8, "Counter8");
    debugger_register_signal(SIGNAL_TEMPERATURE, DEBUGGER_SIGNAL_TYPE_FLOAT, "Temperature");
    debugger_register_signal(SIGNAL_COUNTER16, DEBUGGER_SIGNAL_TYPE_UINT16, "Counter16");
}

void app_init() {
    debugger_interface_t interface = {.transmit_data = &bsp_uart_transmit,
                                      .get_timestamp_ms = &get_timestamp_ms};
    debugger_init(&interface);
    bsp_adc_init();
    bsp_uart_init(debugger_process_data);

    debugger_log_reset(tag, "App start");
}

void app_update() {
    if (bsp_adc_get(&adc_temperature)) {
        float temperature = ntc_calculate_temperature(adc_temperature);
        if (!isnan(temperature)) {
            debugger_plot_signal_float(SIGNAL_TEMPERATURE, temperature);
        }
    }
    if (counter_delay == 1000) {
        debugger_log_info(tag, "This is a test message");
        if (transmit_counter) {
            debugger_plot_signal_uint8(SIGNAL_COUNTER8, counter);
        }
        debugger_plot_signal_uint16(SIGNAL_COUNTER16, counter16);

        counter++;
        counter16++;

        if (counter == 0) {
            transmit_counter = !transmit_counter;
        }
        counter_delay = 0;
    } else {
        counter_delay++;
    }
    if (debugger_get_command(&current_command)) {
        app_command command = (app_command)current_command.command_type;
        switch (command) {
            case APP_COMMAND_REQUEST_REGISTERS:
                debugger_log_info(tag, "Registers requested");
                send_registers();
                break;
            default: {
                char buff[32];
                snprintf(buff, sizeof(buff), "Invalid command with id: %d",
                         current_command.command_type);
                debugger_log_warning(tag, buff);
                break;
            }
        }
    }

    bsp_delay(1);
}
