/*
 *  frame.h
 *
 *  Created on: 20 Aug 2026
 *  Author: jochman03
 */

#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define FRAME_SYNC_BYTE_1 0xA0
#define FRAME_SYNC_BYTE_2 0x0A

#define FRAME_MAX_COMMAND_PAYLOAD 255U
#define FRAME_COMMAND_QUEUE_SIZE  5U

typedef struct frame_t {
    uint8_t command_type;
    uint8_t length;
    uint8_t payload[FRAME_MAX_COMMAND_PAYLOAD];
} frame_t;

typedef struct frame_parser_t {
    uint8_t parser_state;
    frame_t parser_command;
    uint8_t parser_payload_index;
    frame_t* command_queue;
    uint8_t command_queue_size;
    volatile uint8_t command_queue_head;
    volatile uint8_t command_queue_tail;
} frame_parser_t;

bool frame_parser_init(frame_parser_t* parser, frame_t* command_queue, uint8_t command_queue_size);

size_t frame_encode(uint8_t* data, uint8_t command_type, const uint8_t* payload, uint8_t length);
void frame_decode(frame_parser_t* parser, const uint8_t* data, size_t length);
bool frame_parser_get(frame_parser_t* parser, frame_t* frame);
uint8_t frame_crc8(const uint8_t* data, size_t length);

#endif /* FRAME_H */
