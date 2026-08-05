#include "bsp.h"
#include "main.h"

void bsp_delay(size_t delay) {
    HAL_Delay(delay);
}

uint32_t get_timestamp_ms() {
    return HAL_GetTick();
}
