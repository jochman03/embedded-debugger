/*
 *  ntc.h
 *
 *  Created on: 23 Jul 2026
 *  Author: jochman03
 */

#ifndef NTC_H
#define NTC_H

#include <stdint.h>

void ntc_init(float adc_max, float fixed_resistance, float nominal_resistance, float beta,
              float nominal_temperature);

float ntc_calculate_temperature(uint16_t adc_value);

#endif /* NTC_H */
