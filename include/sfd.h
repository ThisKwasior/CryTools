#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fd.h>
#include <mpeg.h>
#include <m1v.h>
#include <adx.h>

#include <io_common.h>

/*
	The amazing resource that made anything here possible
	is STN Software's writeup of DVD-Video
	http://stnsoft.com/DVD/mpeghdrs.html
	Kudos to them!
*/

/*
	SFD - Sofdec multimedia format
	
	It's a standard MPEG-PS container
	with m1v/m2v video and ADX/AIX/AHX/AC3 audio.
	
	= Important stuff about MPEG-PS:
	
	All packets in MPEG-PS should be aligned to 0x800/2048 boundary.
	Every packet starts with 0x000001 + Start code/Stream ID (like 0xBA).
	Every 0x800 block should start with Pack Header (0xBA).
	Stream packets have a maximum size of 0x7FA/2042.
	
	= Important stuff about SFD:
	
	There are a few quirks regarding Criware's take on MPEG-PS
	AND mpeg streams.
	
	Audio takes priority in SFDs (first media packet in any file I look).
	ADX STD buffer size field is constant (0x4004).
	
	General packet flow of SFD file:
	- Pack Header (0xBA)
	- System Header (0xBB) - Stream_bound entries contain audio only
	- Padding Stream (0xBE) - To align to 0x800
	- Pack Header (0xBA)
	- System Header (0xBB) - Stream_bound entries contain video only
	- Padding Stream (0xBE) - To align to 0x1000
	- Pack Header (0xBA)
	- Private Stream 2 (0xBF) - Constains metadata used by game's decoder.
		Some games need it, some games don't. I have no idea how
		it works. It has two versions - "SofdecStream" and "SofdecStream2".
		Structure layout for these is later down this file.
		Search for "Metadata".
	- Pack Header (0xBA)
	- Private Stream 2 (0xBF) - Human-readable info about video stream(s).
		Isn't needed I think, but I guess it won't hurt to write some silly
		things in here.
		Nevermind it's NEEDED SOMETIMES
	- Padding Stream (0xBE) - To align to 0x2000
	
	# Flow here is variable.
	# Think of it as a loop until files have data.
	# All packets start with Pack Header still.
	# If packets don't have enough data to fill entire 0x7FA buffer,
	# Padding Stream is used.
	- Audio packets from 0xC0 to 0xDF
	- Video packets from 0xE0 to 0xEF
	
	# File end
	- Program end (0xB9)
	
*/

/*
	Defines
*/

#define SFD_STREAM_AUDIO_TYPE_ADX = 0x00 // ADX/SFA
#define SFD_STREAM_AUDIO_TYPE_AC3 = 0x02 // AC3 (Dolby Digital)
#define SFD_STREAM_AUDIO_TYPE_AHX = 0x03 // AHX
#define SFD_STREAM_AUDIO_TYPE_AIX = 0x04 // AIX

#define SFD_STREAM2_AUDIO_TYPE_ADX = 0x01 // ADX/SFA
#define SFD_STREAM2_AUDIO_TYPE_AIX = 0x03 // AIX

/*
	Lookup tables
*/

static const uint8_t sfd_csc_y_lut[256] =
{
	0x00, 0x01, 0x03, 0x05, 0x06, 0x08, 0x09, 0x0B, 0x0C, 0x0E, 0x0F, 0x11, 0x12, 0x14, 0x15, 0x17,
	0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F, 
	0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 
	0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 
	0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x37, 
	0x38, 0x38, 0x39, 0x39, 0x3A, 0x3A, 0x3B, 0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3E, 0x3E, 0x3F, 0x3F, 
	0x40, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 
	0x48, 0x48, 0x49, 0x49, 0x4A, 0x4A, 0x4B, 0x4B, 0x4C, 0x4C, 0x4D, 0x4D, 0x4E, 0x4E, 0x4F, 0x4F, 
	0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 
	0x58, 0x58, 0x59, 0x59, 0x5A, 0x5A, 0x5B, 0x5B, 0x5C, 0x5C, 0x5D, 0x5D, 0x5E, 0x5E, 0x5F, 0x5F, 
	0x60, 0x60, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x67, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
	0x78, 0x7A, 0x7C, 0x7E, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8A, 0x8C, 0x8E, 0x90, 0x92, 0x94, 0x96,
	0x98, 0x9A, 0x9C, 0x9E, 0xA0, 0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC, 0xAE, 0xB0, 0xB2, 0xB4, 0xB6,
	0xB8, 0xBA, 0xBC, 0xBE, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE, 0xD0, 0xD2, 0xD4, 0xD6,
	0xD8, 0xDA, 0xDD, 0xDF, 0xE2, 0xE5, 0xE7, 0xEA, 0xEC, 0xEF, 0xF2, 0xF4, 0xF7, 0xF9, 0xFC, 0xFF

};

static const uint8_t sfd_csc_uv_lut[256] =
{
	0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x21,
	0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2B, 0x2C, 0x2C, 0x2E, 0x2F, 0x30, 0x31, 0x31,
	0x32, 0x34, 0x35, 0x35, 0x36, 0x38, 0x38, 0x39, 0x3A, 0x3C, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
	0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51,
	0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61,
	0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
	0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x79, 0x7A, 0x7A, 0x7A, 0x7A, 0x7B, 0x7B, 0x7B,
	0x7C, 0x7C, 0x7C, 0x7D, 0x7D, 0x7D, 0x7E, 0x7E, 0x7E, 0x7E, 0x80, 0x7F, 0x7F, 0x7F, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x80, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x83, 0x84, 0x84,
	0x84, 0x85, 0x85, 0x85, 0x85, 0x86, 0x86, 0x86, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D,
	0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
	0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA6, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD,
	0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
	0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC8, 0xCA, 0xCB, 0xCC, 0xCD,
	0xCE, 0xCF, 0xCF, 0xD0, 0xD2, 0xD3, 0xD3, 0xD4, 0xD6, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDC, 0xDC,
	0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE3, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE9, 0xE9, 0xEA, 0xEB, 0xEC
};

/*
	"SofdecStream            " Metadata
*/

typedef union SFD_METADATA_FILE_ENTRY
{
	struct __attribute__((__packed__)) SFD_AUDIO_ENTRY
	{
		uint8_t out_name[8]; // = {0x20};
		uint8_t out_ext[4]; // = {'.', 's', 'f', 'd'};
		uint8_t out_date[12]; //YYYYMMDDhhmm
		uint8_t stream_id; // 0xC0
		uint8_t audio_type; // 0x00 for SFA/ADX
							// 0x02 for AC3 (Dolby Digital)
							// 0x03 for AHX
							// 0x04 for AIX
							
		uint8_t padding_0x1A; // = 0
		uint8_t channels;
		uint32_t sample_rate; // LE, sample rate
		uint8_t padding_0x20[0x20]; // = {0}
	} audio;
	
	struct __attribute__((__packed__)) SFD_VIDEO_ENTRY
	{
		uint8_t out_name[8]; // = {0x20}; // If longer than 8, just copy 8
		uint8_t out_ext[4]; // = {'.', 's', 'f', 'd'};
		uint8_t out_date[12]; //YYYYMMDDhhmm
		uint8_t stream_id; // 0xE0
		uint8_t stream_id_01; // 0x01
		uint8_t padding_0x1A[2]; // = {0xFF, 0xFF};
		uint8_t width; // Only first byte of 16bit width. Weird, because height is full 16bit 
		uint16_t height; // BE
		uint8_t padding_0x20[0x20];
	} video;
	
	struct __attribute__((__packed__)) SFD_SOFDEC_ENTRY
	{
		uint8_t out_name[8]; // = {0x20}; // If longer than 8, just copy 8
		uint8_t out_ext[4]; // = {'.', 's', 'f', 'd'};
		uint8_t out_date[12]; //YYYYMMDDhhmm
		uint8_t padding[8]; // = {0}
		uint8_t muxer_name[0x20]; // = {0x20}
	} sofdec;
	
	uint8_t data[0x40];
	
} sfd_metadata_file_s;

typedef struct __attribute__((__packed__)) SFD_STREAM
{
	uint8_t padding1[0x0E]; // = {0};
	uint8_t sofdec_string[0x18]; // = "SofdecStream            ";
	uint8_t unk_0x26; // = 0x02; // Always 0x02 I think?
	uint8_t unk_0x27; // = 0x1C; // I've seen it as 0x1C, 0x19, could be random
	uint8_t padding_0x28[0x06]; // = {0};
	
	sfd_metadata_file_s out_file;
	
	uint32_t mpeg_pack_size; // LE, usually 2048
	uint32_t unk_0x70; // = 0x00000000
	uint32_t unk_0x76; // = 0x02000000
	uint32_t mpeg_pack_size_again; // LE, the same as mpeg_pack_size, whyyyy
	uint8_t padding_0x7E[0x20]; // = {0};
	
	uint8_t total_streams;
	uint8_t audio_streams;
	uint8_t video_streams;
	uint8_t private_streams; // = 0x01;
	
	uint32_t unk_0xA2; // LE, gets bigger with more streams
	uint32_t longest_audio_len; // LE, length in ms of longest audio stream
	uint32_t longest_video_len; // LE, length in ms of longest video stream
	uint32_t max_num_video_frames; // LE
	uint32_t max_size_pictures; // LE
	uint32_t avg_size_pictures; // LE
	uint8_t padding_0xBA[0x14]; // = {0};
	
	uint8_t user_comment_0xCE[0x40]; // = {0x20};
	
	uint8_t padding_0x10E[0x60]; // = {0};
	
	sfd_metadata_file_s reserved_stream;
	sfd_metadata_file_s stream_entries[25];
	
} sfd_stream_s;

/*
	"SofdecStream2           " Metadata
*/

typedef struct __attribute__((__packed__)) SFD_AUDIO_ENTRY_STREAM2
{
	uint8_t stream_id;
	
	uint8_t unk_0x01_0 : 4; // 2
	uint8_t audio_format : 4; // 1 for ADX, 3 for AIX
	
	uint8_t channels : 4; // Audio channels in a file
	uint8_t unk_0x02_4 : 4; // 0
	
	uint16_t sample_rate; // BE
	
	uint32_t samplerate_x_duration;	// BE, first byte of sample_rate 
									// times duration in seconds.
									// Beats me what is it for.
									// For example, sample rate is 0xBB80, 0xBB is 187
									// 187 x 60 = 11220
	
	uint8_t unk_0x09; // some files have value in there
	uint8_t unk_0x0A; // = 0

	uint16_t unk_0x0B; // BE, some files have value in there
	uint8_t unk_0x0D; // second byte of sample_rate
	
	uint16_t unk_0x0E; // = 0x0000

} sfd_stream2_audio_metadata_s;

typedef struct __attribute__((__packed__)) SFD_VIDEO_ENTRY_STREAM2
{
	uint8_t stream_id;
	uint8_t unk_0x01; // 0x41
	uint16_t width; // BE, width of video
	uint16_t height; // BE, height of video
	
	uint32_t frames; // BE, amount of frames in video
	uint16_t framerate; // BE, framerate*1000
	
	uint32_t unk_0x0C; // BE
	uint32_t unk_0x10; // BE
	uint8_t padding_0x14[0x0C];
	
	uint16_t unk_0x20; // BE, 0x0100
	uint16_t unk_0x22; // BE, 0x0010
	uint16_t unk_0x24; // BE, 0x000F
	uint16_t unk_0x26; // BE, 0x0300
	uint16_t unk_0x28; // BE, something with width of a frame
	uint16_t unk_0x2A; // BE, something with height of a frame
	uint8_t padding[0x14];
	
} sfd_stream2_video_metadata_s;

typedef struct __attribute__((__packed__)) SFD_STREAM2
{
	uint16_t mpeg_pack_size; // BE, = 2048
	uint8_t padding_0x02[0x0C]; // = {0};
	uint8_t sofdec_string[0x18]; // = "SofdecStream2           ";
	
	/* 
	Could be some kind of muxer/encoder representation.
	Muxers go as follows:
		Gens			- "CRI Sofdec Craft Ver.4.07.00"
		Colors			- "CRI Sofdec Craft Ver.4.07.00"
		Unleashed		- "SFM Ver.2.39 2008-06-04 CRI-MW  "
		Metroid Other M	- "CRI Sofdec Craft Ver.3.00.00"		*/
	/*					 Gens	Colors	Other M		Unleashed	*/
	uint8_t unk_0x26; // 0x02	0x02	0x02		0x02
	uint8_t unk_0x27; // 0x03	0x03	0x03		0x01
	uint8_t unk_0x28; // 0x02	0x02	0x02		0x02
	uint8_t unk_0x29; // 0x49	0x33	0x33		0x27
	uint8_t padding_0x2A[0x04];	// = {0}
	uint8_t muxer_name[0x20]; 	// = {0x20}
	
	uint8_t padding_0x4E[0x60];	// = {0}
								// Could be a comment here, like stream1
								
	uint8_t total_streams;
	uint8_t audio_streams;
	uint8_t video_streams;
	uint8_t private_streams; // = 0x01;
								
	uint32_t unk_0xB2;	// BE
	uint32_t longest_audio_len;	// BE, length in ms of longest audio stream
	uint32_t longest_video_len;	// BE, length in ms of longest video stream
	uint32_t max_num_video_frames;	// BE
	uint32_t unk_0xC2;	// BE
	uint32_t unk_0xC6;	// BE
	uint32_t unk_0xCA;	// BE, never bigger than 255
	uint8_t padding_0xCE[0xE0];	// = {0};
	
	sfd_stream2_audio_metadata_s audio_entries[32];
	sfd_stream2_video_metadata_s video_entries[16];
	
	uint8_t padding_0x7AE[0x20];	// = {0};
	uint8_t unk_0x7CE; // = 0xBF
	uint8_t padding_0x7CF[0x1F];	// = {0};
	
} sfd_stream2_s;

/*
	Before every frame, there's some user data inserted by
	Sofdec encoder. Its contents are a mystery to me, but after
	"<SUDPS_><000006>02A" or "<SUDPS_><000005>02A"
	there's one byte that specifies the conversion mode for YUV components:
		- "N" = normal, bt601 matrix (i guess)
		- "C" = color space compensation (csc), 
				custom matrices specified by trial-and-error
			    in sfd_csc_*_lut[256]
				
	Fun fact - some games won't play video without this data present.
	RE: The Darkside Chronicles needs csc frames for example.
*/
typedef struct SFD_FRAME_METADATA
{
	uint8_t* data;
	uint16_t size;
} sfd_frame_metadata_s;

/*
	Functions
*/

void sfd_sofdec_mpeg_packet(File_desc* fd, const uint8_t fd_size);

void sfd_sofdec2_mpeg_packet(File_desc* fd, const uint8_t fd_size);

void sfd_m1v_insert_userdata(sfd_frame_metadata_s* metadata, M1v_info* m1v);

/*
	Look up SFD_FRAME_METADATA
*/
uint8_t sfd_csc_to_raw_y(const uint8_t value);
uint8_t sfd_csc_to_raw_uv(const uint8_t value);