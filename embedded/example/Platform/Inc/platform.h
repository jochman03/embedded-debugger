/*
 *  platform.h
 *
 *  Created on: 22 Jul 2026
 *  Author: jochman03
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stddef.h>

void platform_delay(size_t delay);
uint32_t platform_get_timestamp_ms();

#endif /* PLATFORM_H */