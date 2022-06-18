#pragma once

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct FileDescriptor
{
	FILE* f;
	uint8_t is_adx;
	uint8_t is_aix;
	uint8_t is_mpeg;
	uint8_t is_output;
	
	uint64_t file_size;
	uint32_t stream_id;
	
	float adx_scr_step;
	float adx_cur_scr;
	uint32_t adx_frame_count;
	uint8_t channel_count;
	uint16_t sample_rate;
	
	uint8_t file_done;
	
} File_desc;

/*
	Reading
*/

void io_read_bits(const uint8_t* data, uint8_t* buffer, const uint32_t buffer_start_bit,
				  const uint32_t data_start_bit, const uint32_t data_bits_to_read);

uint8_t io_read_uint8(const uint8_t* data, uint64_t* data_pos);

uint16_t io_read_uint16(const uint8_t* data, uint64_t* data_pos);

uint32_t io_read_uint32(const uint8_t* data, uint64_t* data_pos);

uint64_t io_read_uint64(const uint8_t* data, uint64_t* data_pos);

float io_read_float(const uint8_t* data, uint64_t* data_pos);

double io_read_double(const uint8_t* data, uint64_t* data_pos);

uint8_t* io_read_array(const uint8_t* data, const uint64_t data_size, uint64_t* data_pos);

/*
	Writing
*/


void io_write_uint8(const uint8_t number, uint8_t* data, uint64_t* data_pos);

void io_write_uint16(const uint16_t number, uint8_t* data, uint64_t* data_pos);

void io_write_uint32(const uint32_t number, uint8_t* data, uint64_t* data_pos);

void io_write_uint64(const uint64_t number, uint8_t* data, uint64_t* data_pos);

void io_write_float(const float number, uint8_t* data, uint64_t* data_pos);

void io_write_double(const double number, uint8_t* data, uint64_t* data_pos);

void io_write_array(const uint8_t* array, const uint64_t array_size, uint8_t* data, uint64_t* data_pos);