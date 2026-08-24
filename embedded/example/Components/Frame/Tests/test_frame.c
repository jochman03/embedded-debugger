#include <string.h>

#include "unity.h"
#include "frame.h"

#define TEST_BUFFER_SIZE 64U

static frame_t command_queue[FRAME_COMMAND_QUEUE_SIZE];
static frame_parser_t parser;
static uint8_t test_buffer[TEST_BUFFER_SIZE];

void setUp(void) {
    memset(command_queue, 0U, sizeof(command_queue));
    memset(&parser, 0U, sizeof(parser));
    memset(test_buffer, 0U, sizeof(test_buffer));
}

void tearDown(void) {}

static void test_init_null_parser(void) {
    TEST_ASSERT_FALSE(frame_parser_init(NULL, command_queue, FRAME_COMMAND_QUEUE_SIZE));
}
static void test_init_null_queue(void) {
    TEST_ASSERT_FALSE(frame_parser_init(&parser, NULL, FRAME_COMMAND_QUEUE_SIZE));
}
static void test_init_zero_size(void) {
    TEST_ASSERT_FALSE(frame_parser_init(&parser, command_queue, 0U));
}
static void test_valid_init(void) {
    TEST_ASSERT_TRUE(frame_parser_init(&parser, command_queue, FRAME_COMMAND_QUEUE_SIZE));
}

static void test_encode_decode_get(void) {
    TEST_ASSERT_TRUE(frame_parser_init(&parser, command_queue, FRAME_COMMAND_QUEUE_SIZE));

    const uint8_t payload[] = {0x01U, 0x32U};
    const size_t encoded_length = frame_encode(test_buffer, 2U, payload, (uint8_t)sizeof(payload));

    TEST_ASSERT_EQUAL_size_t(sizeof(payload) + 5U, encoded_length);
    TEST_ASSERT_EQUAL_UINT8(FRAME_SYNC_BYTE_1, test_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(FRAME_SYNC_BYTE_2, test_buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(2U, test_buffer[2]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)sizeof(payload), test_buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(frame_crc8(&test_buffer[2], sizeof(payload) + 2U),
                            test_buffer[encoded_length - 1U]);

    frame_decode(&parser, test_buffer, encoded_length);

    frame_t frame = {0};
    TEST_ASSERT_TRUE(frame_parser_get(&parser, &frame));
    TEST_ASSERT_EQUAL_UINT8(2U, frame.command_type);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)sizeof(payload), frame.length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, frame.payload, sizeof(payload));
    TEST_ASSERT_FALSE(frame_parser_get(&parser, &frame));
}

static void test_decode_parted(void) {
    TEST_ASSERT_TRUE(frame_parser_init(&parser, command_queue, FRAME_COMMAND_QUEUE_SIZE));

    const uint8_t payload[] = {0xAAU};
    const size_t encoded_length = frame_encode(test_buffer, 1U, payload, (uint8_t)sizeof(payload));

    for (size_t i = 0U; i < encoded_length; ++i) {
        frame_decode(&parser, &test_buffer[i], 1U);
    }

    frame_t frame = {0};
    TEST_ASSERT_TRUE(frame_parser_get(&parser, &frame));
    TEST_ASSERT_EQUAL_UINT8(1U, frame.command_type);
    TEST_ASSERT_EQUAL_UINT8(1U, frame.length);
    TEST_ASSERT_EQUAL_UINT8(0xAAU, frame.payload[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_null_parser);
    RUN_TEST(test_init_null_queue);
    RUN_TEST(test_init_zero_size);
    RUN_TEST(test_valid_init);
    RUN_TEST(test_encode_decode_get);
    RUN_TEST(test_decode_parted);

    return UNITY_END();
}
