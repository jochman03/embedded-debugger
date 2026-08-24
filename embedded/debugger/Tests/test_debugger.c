#include <string.h>
#include "unity.h"

#include "debugger.h"

#define TEST_BUFFER_SIZE 256U
#define TEST_TIMESTAMP   0x12345678U

static uint8_t test_buffer[TEST_BUFFER_SIZE];
static size_t test_length = 0U;
static uint8_t test_frame_type = 0xFFU;

static void transmit_dummy(uint8_t frame_type, const uint8_t* payload, uint8_t length) {
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(test_buffer), length);
    test_frame_type = frame_type;
    memcpy(test_buffer, payload, length);
    test_length = length;
}

static uint32_t timestamp_dummy(void) {
    return TEST_TIMESTAMP;
}

static void initialize_debugger(void) {
    const debugger_interface_t interface = {.transmit = &transmit_dummy,
                                            .get_timestamp_ms = timestamp_dummy};
    TEST_ASSERT_TRUE(debugger_init(&interface));
}

void setUp(void) {
    memset(test_buffer, 0U, sizeof(test_buffer));
    test_length = 0U;
    test_frame_type = 0xFFU;
}

void tearDown(void) {}

static void test_init_null_interface(void) {
    TEST_ASSERT_FALSE(debugger_init(NULL));
}

static void test_init_null_transmit(void) {
    const debugger_interface_t interface = {.transmit = NULL, .get_timestamp_ms = timestamp_dummy};

    TEST_ASSERT_FALSE(debugger_init(&interface));
}

static void test_valid_init(void) {
    initialize_debugger();
}

static void test_plot_unregistered_signal(void) {
    initialize_debugger();
    const uint8_t value = 42U;
    debugger_plot_signal(1U, &value);

    TEST_ASSERT_EQUAL_size_t(0U, test_length);
}

static void test_plot_null_value(void) {
    initialize_debugger();
    debugger_register_signal(1U, DEBUGGER_SIGNAL_TYPE_INT16, "null");
    test_length = 0U;
    debugger_plot_signal(1U, NULL);

    TEST_ASSERT_EQUAL_size_t(0U, test_length);
}

static void test_plot_reinit(void) {
    initialize_debugger();
    debugger_register_signal(1, DEBUGGER_SIGNAL_TYPE_INT16, "null");
    test_length = 0U;
    initialize_debugger();
    const uint16_t value = 1500U;
    debugger_plot_signal(1U, &value);

    TEST_ASSERT_EQUAL_size_t(0U, test_length);
}

static void test_plot_wrong_type(void) {
    initialize_debugger();
    debugger_register_signal(1U, (debugger_signal_type)244, "signal");
    const uint16_t value = 1500U;
    debugger_plot_signal(1U, &value);

    TEST_ASSERT_EQUAL_size_t(0U, test_length);
}

static void test_register_signal(void) {
    initialize_debugger();
    const uint8_t expected[] = {0x01U, DEBUGGER_SIGNAL_TYPE_UINT16, 0x05U, 's', 'p', 'e', 'e', 'd'};
    debugger_register_signal(1, DEBUGGER_SIGNAL_TYPE_UINT16, "speed");

    TEST_ASSERT_EQUAL_UINT8(DEBUGGER_FRAME_TYPE_REGISTER, test_frame_type);
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_plot_uint8(void) {
    initialize_debugger();
    debugger_register_signal(1U, DEBUGGER_SIGNAL_TYPE_UINT8, "name");
    test_length = 0U;
    const uint8_t expected[] = {0x01U, 0x78U, 0x56U, 0x34U, 0x12U, 0xA8U};
    const uint8_t value = 168U;
    debugger_plot_signal(1U, &value);

    TEST_ASSERT_EQUAL_UINT8(DEBUGGER_FRAME_TYPE_PLOT, test_frame_type);
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_log(void) {
    initialize_debugger();
    const uint8_t expected[] = {
        0x06U, 0x03U, 0x05U, 0x78U, 0x56U, 0x34U, 0x12U, '[', 'I', 'N', 'F',
        'O',   ']',   'A',   'P',   'P',   'R',   'e',   'a', 'd', 'y',
    };
    debugger_log_info("APP", "Ready");

    TEST_ASSERT_EQUAL_UINT8(DEBUGGER_FRAME_TYPE_LOG, test_frame_type);
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_null_interface);
    RUN_TEST(test_init_null_transmit);
    RUN_TEST(test_valid_init);
    RUN_TEST(test_register_signal);
    RUN_TEST(test_plot_unregistered_signal);
    RUN_TEST(test_plot_null_value);
    RUN_TEST(test_plot_reinit);
    RUN_TEST(test_plot_wrong_type);
    RUN_TEST(test_plot_uint8);
    RUN_TEST(test_log);

    return UNITY_END();
}