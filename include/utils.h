#pragma once

#include <stdio.h>
#include <stdint.h>
#include "common.h"

/*
	Deprecated

// 32bit BE to LE and vice versa
#define CHGORD32(x) ((x&0x000000FF)<<24 | (x & 0x0000FF00)<<8 | (x & 0x00FF0000)>>8 | (x & 0xFF000000)>>24)

// 16bit BE to LE and vice versa
#define CHGORD16(x) (x<<8 | x>>8)
*/

// You can modify x'th bit all you want
#define BIT(x) (1<<(x))

void print_binary(uint8_t* arr, uint32_t size);