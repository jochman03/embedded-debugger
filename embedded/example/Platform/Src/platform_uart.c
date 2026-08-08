#include <stdint.h>

#include "platform_uart.h"
#include "dma.h"
#include "usart.h"

UART_HandleTypeDef* platform_uart = &huart2;

uint8_t platform_uart_rx_buffer[PLATFORM_UART_BUFFER_SIZE];

static volatile size_t platform_uart_rx_position;
static platform_uart_receive_callback_t platform_uart_receive_callback;

void platform_uart_transmit(const uint8_t* data, size_t length) {
    HAL_UART_Transmit(platform_uart, data, (uint16_t)length, HAL_MAX_DELAY);
}

void platform_uart_init(platform_uart_receive_callback_t receive_callback) {
    if (receive_callback == NULL) {
        return;
    }

    platform_uart_receive_callback = receive_callback;
    platform_uart_rx_position = 0;

    HAL_UARTEx_ReceiveToIdle_DMA(platform_uart, platform_uart_rx_buffer,
                                 sizeof(platform_uart_rx_buffer));
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* uart, uint16_t position) {
    if ((uart != platform_uart) || (platform_uart_receive_callback == NULL)) {
        return;
    }

    if (position > platform_uart_rx_position) {
        platform_uart_receive_callback(&platform_uart_rx_buffer[platform_uart_rx_position],
                                       (size_t)position - platform_uart_rx_position);
    }

    else if (position < platform_uart_rx_position) {
        platform_uart_receive_callback(&platform_uart_rx_buffer[platform_uart_rx_position],
                                       PLATFORM_UART_BUFFER_SIZE - platform_uart_rx_position);

        if (position > 0) {
            platform_uart_receive_callback(platform_uart_rx_buffer, position);
        }
    }

    platform_uart_rx_position = position;

    if (platform_uart_rx_position == PLATFORM_UART_BUFFER_SIZE) {
        platform_uart_rx_position = 0;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if (huart != platform_uart) {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(huart);

    platform_uart_rx_position = 0;

    HAL_UARTEx_ReceiveToIdle_DMA(platform_uart, platform_uart_rx_buffer,
                                 sizeof(platform_uart_rx_buffer));
}