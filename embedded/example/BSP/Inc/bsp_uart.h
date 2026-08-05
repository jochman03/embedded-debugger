/*
 *  bsp_uart.h
 *
 *  Created on: 22 Jul 2026
 *  Author: jochman03
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stddef.h>
#include "usart.h"

#define BSP_UART_BUFFER_SIZE 64

extern uint8_t bsp_uart_rx_buffer[BSP_UART_BUFFER_SIZE];

typedef void (*bsp_uart_receive_callback_t)(const uint8_t* data, size_t length);

void bsp_uart_transmit(const uint8_t* data, size_t length);
void bsp_uart_init(bsp_uart_receive_callback_t receive_callback);

#endif /* BSP_UART_H */