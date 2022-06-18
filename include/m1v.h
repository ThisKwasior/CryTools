#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <common.h>

#define M1V_SEQ_HEAD	0x000001B3	// Sequence Header
#define M1V_GOP			0x000001B8	// Group of Pictures
#define M1V_PIC_HEAD	0x00000100	// Picture header
#define M1V_EXTENSION	0x000001B5	// Extension
#define M1V_USER_DATA	0x000001B2	// User data
#define M1V_SLICE_FIRST	0x01		// First slice stream id
#define M1V_SLICE_LAST	0xAF		// Last slice stream id

#define M1V_FRAME_I		1
#define M1V_FRAME_P		2
#define M1V_FRAME_B		3
#define M1V_FRAME_D		4

static const float mpeg1_par[] = {-1.f, 1.f, 0.6735f, 0.7031f,
								  0.7615f, 0.8055f, 0.8437f, 0.8935f,
								  0.9375f, 0.9815f, 1.0255f, 1.0695f,
								  1.1250f, 1.1575f, 1.2015f, -1.f};

static const float mpeg2_dar[] = {-1.f, 1.f, 4/3.f, 16/9.f,
								  2.21f/1.f, -1.f, -1.f, -1.f,
								  -1.f, -1.f, -1.f, -1.f,
								  -1.f, -1.f, -1.f, -1.f};

static const float mpeg1_framerate[] = {-1.f, 24000/1001.f, 24.f, 25.f,
										30000/1001.f, 30.f, 50.f, 60000/1001.f,
										60.f, -1.f, -1.f, -1.f,
										-1.f, -1.f, -1.f, -1.f};

typedef struct M1V_INFO
{
	uint8_t codec : 2; // 1 = m1v, 2 = m2v
	uint8_t last_stream_id;
	uint64_t file_size;
	uint64_t file_pos;
	uint8_t* file_buffer;
	
	// Sequence header
	uint16_t width : 12; // Horizontal size
	uint16_t height : 12; // Vertical size
	uint8_t aspect_ratio : 4;
	uint8_t frame_rate : 4;
	uint32_t bitrate : 18;
	uint16_t vbv_buf_size : 10;
	uint8_t constrained_params_flag : 1;
	uint8_t load_intra_quant_matrix : 1;
	uint8_t load_nonintra_quant_matrix : 1;
	uint8_t intra_quant_matrix[64];
	uint8_t nonintra_quant_matrix[64];
	
	// GOP
	uint8_t drop_frame_flag : 1;
	
	uint8_t time_hour : 5;
	uint8_t time_minute : 6;
	uint8_t time_second : 6;
	
	uint8_t gop_frame : 6;
	uint8_t closed_gop : 1;
	uint8_t broken_gop : 1;
	
	// Picture header
	uint16_t temp_seq_num : 10;	// temporal sequence number
	uint8_t frame_type : 3;		// 1=I, 2=P, 3=B, 4=D
	uint16_t vbv_delay;
	
		// If frame_type = 2 (P) or 3 (B)
	uint8_t full_pel_forward_vector : 1;
	uint8_t forward_f_code : 3;
	
		// Appended if frame_type = 3 (B)
	uint8_t full_pel_backward_vector : 1;
	uint8_t backward_f_code : 3;
	
	// User data
	uint8_t* user_data;
	uint64_t user_data_size; // actual data size + 1 for null byte
	
	// Slice
	uint64_t slice_size;
	
	
} M1v_info;

/*
	Takes data and M1v_info pointers.
	Returns bytes read.
*/

uint64_t m1v_next_packet(M1v_info* m1v);

/*
	Checks
*/

uint8_t m1v_is_slice(const uint8_t stream_id);
uint8_t m1v_is_slice_sync(const uint32_t sync);

/*
	m1v packets
*/

uint64_t m1v_sequence_header(const uint8_t* data, M1v_info* m1v);
uint64_t m1v_gop(const uint8_t* data, M1v_info* m1v);
uint64_t m1v_picture_header(const uint8_t* data, M1v_info* m1v);

uint64_t m1v_slice(const uint8_t* data, M1v_info* m1v, const uint8_t slice_id);

uint64_t m1v_user_data(const uint8_t* data, M1v_info* m1v);