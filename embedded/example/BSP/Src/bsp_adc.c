
#include "bsp_adc.h"
#include "adc.h"
#include "tim.h"

#define BSP_ADC_BUFFER_SIZE 16

static uint16_t bsp_adc_buffer[BSP_ADC_BUFFER_SIZE];
static volatile bool bsp_adc_ready = false;

ADC_HandleTypeDef* bsp_adc = &hadc1;
TIM_HandleTypeDef* bsp_adc_tim = &htim3;

void bsp_adc_init() {
    HAL_ADC_Start_DMA(bsp_adc, (uint32_t*)bsp_adc_buffer, 16);
    HAL_TIM_Base_Start(bsp_adc_tim);
}

bool bsp_adc_get(uint16_t* value) {
    if (bsp_adc_ready) {
        uint32_t sum = 0;
        for (size_t i = 0; i < BSP_ADC_BUFFER_SIZE; i++) {
            sum += bsp_adc_buffer[i];
        }
        *value = (uint16_t)(sum / BSP_ADC_BUFFER_SIZE);
        bsp_adc_ready = false;
        return true;
    }
    return false;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == bsp_adc->Instance) {
        bsp_adc_ready = true;
    }
}