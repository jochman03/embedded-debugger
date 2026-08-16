#include <string.h>

#include "unity.h"
#include "debugger.h"

#define TEST_BUFFER_SIZE 256U
#define TEST_TIMESTAMP   0x12345678U

static uint8_t test_buffer[TEST_BUFFER_SIZE];
static size_t test_length = 0U;

static void transmit_dummy(const uint8_t* data, size_t length) {
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(test_buffer), length);

    memcpy(test_buffer, data, length);
    test_length = length;
}
static uint32_t timestamp_dummy(void) {
    return TEST_TIMESTAMP;
}

static void initialize_debugger(void) {
    const debugger_interface_t interface = {.transmit_data = &transmit_dummy,
                                            .get_timestamp_ms = timestamp_dummy};

    TEST_ASSERT_TRUE(debugger_init(&interface));
}

void setUp(void) {
    memset(test_buffer, 0U, sizeof(test_buffer));
    test_length = 0U;
}
void tearDown(void) {}

static void test_init_null_interface(void) {
    TEST_ASSERT_FALSE(debugger_init(NULL));
}

static void test_init_null_transmit(void) {
    const debugger_interface_t interface = {.transmit_data = NULL,
                                            .get_timestamp_ms = timestamp_dummy};

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

    const uint8_t expected[] = {
        DEBUGGER_SYNC_BYTE_1,
        DEBUGGER_SYNC_BYTE_2,
        DEBUGGER_FRAME_TYPE_REGISTER,
        0x01U,
        DEBUGGER_SIGNAL_TYPE_UINT16,
        0x05U,
        's',
        'p',
        'e',
        'e',
        'd',
        0xC1U,
    };

    debugger_register_signal(1, DEBUGGER_SIGNAL_TYPE_UINT16, "speed");

    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_plot_uint8(void) {
    initialize_debugger();
    debugger_register_signal(1U, DEBUGGER_SIGNAL_TYPE_UINT8, "name");

    const uint8_t expected[] = {
        DEBUGGER_SYNC_BYTE_1,
        DEBUGGER_SYNC_BYTE_2,
        DEBUGGER_FRAME_TYPE_PLOT,
        0x01U,
        0x78U,
        0x56U,
        0x34U,
        0x12U,
        0xA8U,
        0xF9U,
    };

    const uint8_t value = 168U;

    debugger_plot_signal(1U, &value);
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_log(void) {
    initialize_debugger();

    uint8_t expected[] = {
        DEBUGGER_SYNC_BYTE_1,
        DEBUGGER_SYNC_BYTE_2,
        DEBUGGER_FRAME_TYPE_LOG,
        0x06U,
        0x03U,
        0x05U,
        0x78U,
        0x56U,
        0x34U,
        0x12U,

        '[',
        'I',
        'N',
        'F',
        'O',
        ']',

        'A',
        'P',
        'P',

        'R',
        'e',
        'a',
        'd',
        'y',
        0x0U,
    };

    expected[sizeof(expected) - 1U] = debugger_crc8(&expected[2U], sizeof(expected) - 3U);

    debugger_log_info("APP", "Ready");

    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_parse_command(void) {
    const char payload[] = "SET_LED 5";
    const uint8_t payload_length = sizeof(payload) - 1U;

    uint8_t frame[] = {DEBUGGER_SYNC_BYTE_1,
                       DEBUGGER_SYNC_BYTE_2,
                       2U,
                       payload_length,

                       'S',
                       'E',
                       'T',
                       '_',
                       'L',
                       'E',
                       'D',
                       ' ',
                       '5',

                       0U};

    frame[sizeof(frame) - 1U] = debugger_crc8(&frame[2U], payload_length + 2U);

    debugger_process_data(frame, sizeof(frame));

    debugger_command_t command = {0};

    TEST_ASSERT_TRUE(debugger_get_command(&command));

    TEST_ASSERT_EQUAL_UINT8(2U, command.command_type);
    TEST_ASSERT_EQUAL_UINT8(2U, command.argc);

    TEST_ASSERT_EQUAL_STRING("SET_LED", (const char*)command.payload);

    TEST_ASSERT_EQUAL_STRING("5", (const char*)&command.payload[8U]);
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
    RUN_TEST(test_parse_command);

    return UNITY_END();
}
