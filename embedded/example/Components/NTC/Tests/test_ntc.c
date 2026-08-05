#include "unity.h"
#include <stdint.h>
#include <math.h>

#include "ntc.h"

void setUp(void) {}

void tearDown(void) {}

static void test_ntc_min_adc_value(void) {
    const float temperature = ntc_calculate_temperature(0);

    TEST_ASSERT_TRUE(isnan(temperature));
}

static void test_ntc_max_adc_value(void) {
    const float temperature = ntc_calculate_temperature(4095);

    TEST_ASSERT_TRUE(isnan(temperature));
}

static void test_ntc_lowest_valid_adc_value(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 527.890f, ntc_calculate_temperature(1));
}

static void test_ntc_highest_valid_adc_value(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -89.988f, ntc_calculate_temperature(4094));
}

static void test_ntc_mid_adc_value(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 24.989f, ntc_calculate_temperature(2048));
}

static void test_custom_parameters(void) {
    ntc_init(1023.0f, 4700.0f, 4700.0f, 3435.0f, 303.15f);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 29.948f, ntc_calculate_temperature(512));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ntc_min_adc_value);
    RUN_TEST(test_ntc_max_adc_value);
    RUN_TEST(test_ntc_lowest_valid_adc_value);
    RUN_TEST(test_ntc_highest_valid_adc_value);
    RUN_TEST(test_ntc_mid_adc_value);
    RUN_TEST(test_custom_parameters);

    return UNITY_END();
}
