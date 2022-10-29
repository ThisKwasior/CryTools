#pragma once

#include <stdint.h>

#define BIT(x) (1<<(x))

uint16_t change_order_16(const uint16_t n);

uint32_t change_order_32(const uint32_t n);

uint64_t change_order_64(const uint64_t n);

// YYYYMMDDhhmm
void get_current_date_as_str(uint8_t* string);