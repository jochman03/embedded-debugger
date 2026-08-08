/*
 *  platform_adc.h
 *
 *  Created on: 23 Jul 2026
 *  Author: jochman03
 */

#ifndef PLATFORM_ADC_H
#define PLATFORM_ADC_H

#include <stdint.h>
#include <stdbool.h>

void platform_adc_init();
bool platform_adc_get(uint16_t* value);

#endif /* PLATFORM_ADC_H */