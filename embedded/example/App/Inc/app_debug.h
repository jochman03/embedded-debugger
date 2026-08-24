/*
 *  app_debug.h
 *
 *  Created on: 23 Aug 2026
 *  Author: jochman03
 */

#ifndef APP_DEBUG_H
#define APP_DEBUG_H

typedef enum app_command {
    APP_COMMAND_REQUEST_REGISTERS = 0x10,
    APP_COMMAND_SET_LED = 0x11
} app_command;

typedef enum signal { SIGNAL_COUNTER8, SIGNAL_TEMPERATURE } signal;

void app_debug_init(void);
void app_debug_poll(void);

#endif /* APP_DEBUG_H */