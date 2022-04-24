#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <adx.h>

/*
	https://en.wikipedia.org/wiki/Packetized_elementary_stream
	http://dvd.sourceforge.net/dvdinfo/index.html
	
    // Packets
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
#define MPEG_MAX_DATA_SIZE 2047

#define STREAM_VIDEO 1
#define STREAM_AUDIO 2

#define PS_PACK_HEADER 0x000001BA
#define PS_SYSTEM_HEADER 0x000001BB
#define PS_PRIV1 0x000001BD
#define PS_PADDING 0x000001BE
#define PS_PRIV2 0x000001BF
#define PS_MPEG_PROGRAM_END 0x000001B9

/*
	Types
*/

struct Mpeg1Frame
{
	uint16_t len;
	int32_t sync;
	uint16_t frame_data_size;
	int8_t stream;
	uint8_t* data;
	uint8_t av_id;
	uint64_t last_scr;
	uint8_t is_adx;
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
	Decodes the MPEG-1 SCR.
	Takes the pointer to array of 5 bytes beginning with 0010b.

	Returns decoded MPEG-1 SCR value.
*/

uint64_t mpeg1_decode_scr(const uint8_t* const scr_array);

/*
	Encodes the MPEG-1 SCR from 33bit number.
	Takes uint64_t with the value to encode
	and uint8_t* array of 5, which will store MPEG-1 SCR value.
*/

void mpeg1_encode_scr(uint8_t* arr, const uint64_t scr_value);

/*
	Write MPEG1 Program End packet.
	Basically the 'end' of the file
*/

void mpeg1_write_prog_end(FILE* f);

/*
	Write MPEG1 Padding packet directly to a file.
	
	`padding size` is the size of padding without
	sync and size. Max size is 0x07FF.
*/
void mpeg1_write_padding(FILE* file, const uint16_t padding_size);