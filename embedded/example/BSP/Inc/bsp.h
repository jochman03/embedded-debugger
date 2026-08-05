/*
 *  bsp.h
 *
 *  Created on: 22 Jul 2026
 *  Author: jochman03
 */

#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include <stddef.h>

void bsp_delay(size_t delay);
uint32_t get_timestamp_ms();

#endif /* BSP_H */