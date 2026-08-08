#include "platform_led.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"

void platform_led_set(bool state) {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, (state ? RESET : SET));
}