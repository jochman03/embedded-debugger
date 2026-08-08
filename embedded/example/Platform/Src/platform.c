#include "platform.h"
#include "main.h"

void platform_delay(size_t delay) {
    HAL_Delay(delay);
}

uint32_t platform_get_timestamp_ms() {
    return HAL_GetTick();
}
