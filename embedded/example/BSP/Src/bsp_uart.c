#include <stdint.h>

#include "bsp_uart.h"
#include "dma.h"
#include "usart.h"

UART_HandleTypeDef* bsp_uart = &huart2;

uint8_t bsp_uart_rx_buffer[BSP_UART_BUFFER_SIZE];

static volatile size_t bsp_uart_rx_position;
static bsp_uart_receive_callback_t bsp_uart_receive_callback;

void bsp_uart_transmit(const uint8_t* data, size_t length) {
    HAL_UART_Transmit(bsp_uart, data, (uint16_t)length, HAL_MAX_DELAY);
}

void bsp_uart_init(bsp_uart_receive_callback_t receive_callback) {
    if (receive_callback == NULL) {
        return;
    }

    bsp_uart_receive_callback = receive_callback;
    bsp_uart_rx_position = 0;

    HAL_UARTEx_ReceiveToIdle_DMA(bsp_uart, bsp_uart_rx_buffer, sizeof(bsp_uart_rx_buffer));
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* uart, uint16_t position) {
    if ((uart != bsp_uart) || (bsp_uart_receive_callback == NULL)) {
        return;
    }

    if (position > bsp_uart_rx_position) {
        bsp_uart_receive_callback(&bsp_uart_rx_buffer[bsp_uart_rx_position],
                                  (size_t)position - bsp_uart_rx_position);
    }

    else if (position < bsp_uart_rx_position) {
        bsp_uart_receive_callback(&bsp_uart_rx_buffer[bsp_uart_rx_position],
                                  BSP_UART_BUFFER_SIZE - bsp_uart_rx_position);

        if (position > 0) {
            bsp_uart_receive_callback(bsp_uart_rx_buffer, position);
        }
    }

    bsp_uart_rx_position = position;

    if (bsp_uart_rx_position == BSP_UART_BUFFER_SIZE) {
        bsp_uart_rx_position = 0;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if (huart != bsp_uart) {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(huart);

    bsp_uart_rx_position = 0;

    HAL_UARTEx_ReceiveToIdle_DMA(bsp_uart, bsp_uart_rx_buffer, sizeof(bsp_uart_rx_buffer));
}