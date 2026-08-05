#include "ntc.h"
#include <math.h>
#include <stdint.h>

static float ntc_adc_max = 4095.0f;
static float ntc_fixed_resistance = 10000.0f;
static float ntc_nominal_resistance = 10000.0f;
static float ntc_beta = 3950.0f;
static float ntc_nominal_temperature = 298.15f;

void ntc_init(float adc_max, float fixed_resistance, float nominal_resistance, float beta,
              float nominal_temperature) {
    ntc_adc_max = adc_max;
    ntc_fixed_resistance = fixed_resistance;
    ntc_nominal_resistance = nominal_resistance;
    ntc_beta = beta;
    ntc_nominal_temperature = nominal_temperature;
}

float ntc_calculate_temperature(uint16_t adc_value) {
    if ((adc_value == 0U) || (adc_value >= 4095U)) {
        return NAN;
    }

    const float ntc_resistance =
        ntc_fixed_resistance * ((float)adc_value / (ntc_adc_max - (float)adc_value));

    const float temperature_kelvin =
        1.0f / ((1.0f / ntc_nominal_temperature) +
                (logf(ntc_resistance / ntc_nominal_resistance) / ntc_beta));

    return temperature_kelvin - 273.15f;
}