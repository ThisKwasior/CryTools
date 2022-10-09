#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <common.h>

/*
	https://en.wikipedia.org/wiki/Packetized_elementary_stream
	http://dvd.sourceforge.net/dvdinfo/index.html
	
    // Packets
	http://stnsoft.com/DVD/mpeghdrs.html
	
    MPEG_program_end   			//0xB9
    pack_start         			//0xBA
    system_header_start			//0xBB
    program_stream_map  		//0xBC
    private_stream_1  			//0xBD
    padding_stream     			//0xBE
    private_stream_2 			//0xBF
    audio_stream        		//0xC0 --> 0xDF
    video_stream        		//0xE0 --> 0xEF
    SL_packetized_stream		//0xFA
    extension_stream	    	//0xFD
*/

/*
	http://www.mpucoder.com/doom9/packhdr.html#MPEG-1%20Pack%20Header
	MPEG-1 Pack Header
	
	http://stnsoft.com/DVD/sys_hdr.html
	0x000001BB
*/

/*
	In MPEG-1 there is no padding in 0xBA pack
*/

#define MPEG_SCR_MUL 90000
#define MPEG_MAX_VIDEO_STREAMS 16
#define MPEG_MAX_AUDIO_STREAMS 32
#define MPEG_VIDEO_START_ID 0x000001E0
#define MPEG_AUDIO_START_ID 0x000001C0
#define MPEG_DVD_BLOCK 2048
#define MPEG_MAX_DATA_SIZE 2047 // 0x7FF

#define STREAM_VIDEO 1
#define STREAM_AUDIO 2

#define PS_PACK_HEADER 0x000001BA
#define PS_SYSTEM_HEADER 0x000001BB
#define PS_PRIV1 0x000001BD
#define PS_PADDING 0x000001BE
#define PS_PRIV2 0x000001BF
#define PS_MPEG_PROGRAM_END 0x000001B9

#define PS_SEQ_HEADER 0x000001B3

/*
	Types
*/

typedef struct Mpeg1Frame
{
	uint16_t len;
	int32_t sync;
	uint16_t frame_data_size;
	int8_t stream;
	uint8_t* data;
	uint8_t av_id;
	uint64_t last_scr : 33;
	uint8_t is_adx;
	
	uint8_t p_std_buf_scale : 1;
	uint16_t p_std_buf_size : 13;
	
	uint64_t last_pts : 33;
	uint64_t last_dts : 33;
} mpeg_frame_s;

struct MpegStreamInfo
{
	uint8_t codec; 	/* 1 = m1v, 2 = m2v */
	
	uint16_t width : 12;
	uint16_t height : 12;
	uint16_t aspect_ratio : 4;
	uint16_t frame_rate : 4;
	uint32_t bitrate : 18;
	uint16_t vbv_buff_size : 10;
	uint8_t const_param_flag : 1;
	uint8_t intra_quant_flag : 1;
	uint8_t non_intra_quant_flag : 1;
	
	uint8_t intra_quant[64];
	uint8_t non_intra_quant[64];
};

/*
	Functions

*/


/*
	TODO:
	Check if audio frame is ADX (0x4004)
*/
void mpeg_get_next_frame(FILE* mpegfile, struct Mpeg1Frame* frame);

/*
	Get info about the m1v/m2v stream

void mpeg_read_info_from_data(uint8_t* data, struct MpegStreamInfo* info);
*/

/*
	Decodes the MPEG SCR/PTS/DTS.
	Takes the pointer to array of 5 bytes beginning with 0010b or 0011b.

	Returns decoded MPEG-1 SCR/PTS/DTS value.
*/
uint64_t mpeg_decode_scr(const uint8_t* const scr_array);

/*
	Encodes the MPEG-1 SCR from 33bit number.
	Takes uint64_t with the value to encode
	and uint8_t* array of 5, which will store MPEG-1 SCR value.
*/
void mpeg_encode_scr(uint8_t* arr, const uint64_t scr_value);

/*
	Encodes the MPEG-1 PTS/DTS from 33bit numbers, part of the extension
	
	`arr` should have space for 10 bytes of information, if both
	pts and dts are being encoded, otherwise 5 if pts only.
	dts being only one is invalid.
*/
uint8_t mpeg_encode_pts_dts(uint8_t* arr, const uint64_t pts, const uint64_t dts);

/*
	Write MPEG1 Program End packet.
	Basically the 'end' of the file
*/
void mpeg_write_prog_end(FILE* file);

/*
	Write MPEG1 Padding packet directly to a file.
	
	`padding size` is the size of padding without
	sync and size. Max size is 0x07FF.
*/
void mpeg_write_padding(FILE* file, const uint16_t padding_size);

/*
	Write MPEG1 Pack Header packet directly to a file.
	
	`scr` is global scr
	`mux_rate` is ignored for now
	
	size of the packet is always 0xC
*/
void mpeg_write_pack_header(FILE* file, const uint64_t scr, const uint32_t mux_rate);

/*
	Write MPEG1 System Header packet directly to a file.
	Pretty naive approach.
	
	`audio_bound` - audio files amount
	`video_bound` - video files amount
	
	returns bytes written
*/
uint16_t mpeg_write_system_header(FILE* file, const uint8_t audio_bound, const uint8_t video_bound);


/*
	stream packet
*/
uint16_t mpeg_prep_stream_packet(uint8_t* buffer, const uint32_t stream_id,
								 const uint8_t* data, const uint64_t data_size, uint16_t available, 
								 const uint8_t buffer_scale, const uint16_t buffer_size,
								 const uint64_t pst, const uint64_t dst);
