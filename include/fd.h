#pragma once

#include <stdint.h>

#include <m1v.h>
#include <adx.h>

#include <pathutils.h>

#define FD_OUTPUT 0
#define FD_ADX 1
#define FD_AIX 2
#define FD_M1V 10

typedef union FILE_DESCRIPTOR_FILE_META
{
	ADX adx;
	M1v_info m1v;
} fd_file_meta;

typedef struct FILE_DESCRIPTOR
{
	FILE* f;
	File_path_s f_path;
	
	/*
		0 - output
		1 - adx
		2 - aix
		10 - m1v/m2v
	*/
	uint8_t file_type;
	
	uint64_t file_size;
	uint64_t file_pos;
	uint8_t* file_buffer;
	
	uint32_t stream_id;
	
	double scr_step;
	uint8_t first_frame;
	uint32_t frame_count;
	uint32_t frame_count_max; // Never change it pls
	double cur_scr;
	double scr_counter;
	
	uint64_t previous_frame;
	
	uint8_t file_done;
	
	//void* data;
	fd_file_meta data;
	
} File_desc;


void file_desc_destroy(File_desc* files, const uint8_t files_amount);