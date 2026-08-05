/*
 *  bsp_adc.h
 *
 *  Created on: 23 Jul 2026
 *  Author: jochman03
 */

#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#include <stdbool.h>

void bsp_adc_init();
bool bsp_adc_get(uint16_t* value);

#endif /* BSP_ADC_H */