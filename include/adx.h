#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <common.h>

/*
	https://en.wikipedia.org/wiki/ADX_(file_format)#Technical_Description
	https://wiki.multimedia.cx/index.php/CRI_ADX_file
*/

const static uint8_t adx_frame_value[] = {0x40, 0x04};

enum EncodingType
{
	COEFF = 0x02,
	STANDARD = 0x03,
	EXP_SCALE = 0x04,
	AHX1 = 0x10,
	AHX2 = 0x11,
};

struct AdxBasicInfo
{
	uint16_t magic; // Should always be LE 0x0080
	uint16_t copyright_offset; // "(c)CRI" string at [copyrightOffset-2] 
	uint8_t enc_type; // Encoding type
	uint8_t block_size;
	uint8_t bit_depth;
	uint8_t channel_count;
	uint32_t sample_rate;
	uint32_t sample_count;
	uint16_t highpass_freq;
	uint8_t version;
	uint8_t flags;
};

struct AdxLoop
{
	uint32_t loop_flag;
	uint32_t loop_start_sample;
	uint32_t loop_start_byte;
	uint32_t loop_end_sample;
	uint32_t loop_end_byte;
};

typedef struct Adx
{
	struct AdxBasicInfo info;
	uint8_t unknown_loop[16]; /* 4 for v3, 16 for v4 */
	struct AdxLoop loop;
	
	double est_length;
	
	uint64_t file_size;
	uint64_t file_pos;
	uint8_t* file_buffer;
	
} ADX;

/*
	Functions 
*/

//ADX read_adx_info(FILE* adx_file);
ADX adx_read_info(const uint8_t* file_path);

/*
	Checks if a file is ADX or AIX
	0 for unknown
	1 for ADX
	2 for AIX
*/
//uint8_t check_adx_file(FILE* adx_file);
uint8_t adx_check_file(const uint8_t* file_path);


void adx_free(ADX* adx);