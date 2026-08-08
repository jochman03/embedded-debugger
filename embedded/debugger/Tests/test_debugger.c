#include <string.h>

#include "unity.h"
#include "debugger.h"

static uint8_t test_buffer[256];
static size_t test_length = 0;

static void transmit_dummy(const uint8_t* data, size_t length) {
    TEST_ASSERT_NOT_NULL(data);

    memcpy(test_buffer, data, length);
    test_length = length;
}
static uint32_t timestamp_dummy(void) {
    return 0x12345678;
}

static void initialize_debugger(void) {
    const debugger_interface_t interface = {.transmit_data = &transmit_dummy,
                                            .get_timestamp_ms = timestamp_dummy};

    TEST_ASSERT_TRUE(debugger_init(&interface));
}

void setUp(void) {
    memset(test_buffer, 0, sizeof(test_buffer));
    test_length = 0;
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

static void test_register_signal(void) {
    initialize_debugger();

    const uint8_t expected[] = {
        DEBUGGER_SYNC_BYTE_1,
        DEBUGGER_SYNC_BYTE_2,
        DEBUGGER_FRAME_TYPE_REGISTER,
        0x01,
        DEBUGGER_SIGNAL_TYPE_UINT16,
        0x05,
        's',
        'p',
        'e',
        'e',
        'd',
        0xC1,
    };

    debugger_register_signal(1, DEBUGGER_SIGNAL_TYPE_UINT16, "speed");

    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_plot_uint8(void) {
    initialize_debugger();

    const uint8_t expected[] = {
        DEBUGGER_SYNC_BYTE_1,
        DEBUGGER_SYNC_BYTE_2,
        DEBUGGER_FRAME_TYPE_PLOT,
        0x01,
        0x78,
        0x56,
        0x34,
        0x12,
        0xA8,
        0xF9,
    };

    debugger_plot_signal_uint8(1, 168);
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_log(void) {
    initialize_debugger();

    const uint8_t expected[] = {
        DEBUGGER_SYNC_BYTE_1,
        DEBUGGER_SYNC_BYTE_2,
        DEBUGGER_FRAME_TYPE_LOG,
        0x04,
        0x03,
        0x05,
        0x78,
        0x56,
        0x34,
        0x12,

        'I',
        'N',
        'F',
        'O',

        'A',
        'P',
        'P',

        'R',
        'e',
        'a',
        'd',
        'y',
        0x53,
    };

    debugger_log_info("APP", "Ready");

    TEST_ASSERT_EQUAL_size_t(sizeof(expected), test_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, test_buffer, sizeof(expected));
}

static void test_parse_command(void) {
    const char payload[] = "SET_LED 5";
    const uint8_t payload_length = sizeof(payload) - 1;

    uint8_t frame[] = {DEBUGGER_SYNC_BYTE_1,
                       DEBUGGER_SYNC_BYTE_2,
                       2,
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

                       0};

    frame[sizeof(frame) - 1] = debugger_crc8(&frame[2], payload_length + 2);

    debugger_process_data(frame, sizeof(frame));

    debugger_command_t command = {0};

    TEST_ASSERT_TRUE(debugger_get_command(&command));

    TEST_ASSERT_EQUAL_UINT8(2, command.command_type);
    TEST_ASSERT_EQUAL_UINT8(2, command.argc);

    TEST_ASSERT_EQUAL_STRING("SET_LED", (const char*)command.payload);

    TEST_ASSERT_EQUAL_STRING("5", (const char*)&command.payload[8]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_null_interface);
    RUN_TEST(test_init_null_transmit);
    RUN_TEST(test_valid_init);
    RUN_TEST(test_register_signal);
    RUN_TEST(test_plot_uint8);
    RUN_TEST(test_parse_command);

    return UNITY_END();
}
