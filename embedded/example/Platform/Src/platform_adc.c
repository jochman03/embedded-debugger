
#include "platform_adc.h"
#include "adc.h"
#include "tim.h"

#define PLATFORM_ADC_BUFFER_SIZE 16

static uint16_t platform_adc_buffer[PLATFORM_ADC_BUFFER_SIZE];
static volatile bool platform_adc_ready = false;

ADC_HandleTypeDef* platform_adc = &hadc1;
TIM_HandleTypeDef* platform_adc_tim = &htim3;

void platform_adc_init() {
    HAL_ADC_Start_DMA(platform_adc, (uint32_t*)platform_adc_buffer, 16);
    HAL_TIM_Base_Start(platform_adc_tim);
}

bool platform_adc_get(uint16_t* value) {
    if (platform_adc_ready) {
        uint32_t sum = 0;
        for (size_t i = 0; i < PLATFORM_ADC_BUFFER_SIZE; i++) {
            sum += platform_adc_buffer[i];
        }
        *value = (uint16_t)(sum / PLATFORM_ADC_BUFFER_SIZE);
        platform_adc_ready = false;
        return true;
    }
    return false;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == platform_adc->Instance) {
        platform_adc_ready = true;
    }
}