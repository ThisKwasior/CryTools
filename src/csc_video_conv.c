/*
	Converter for videos with "Color space compensation"
	which are SFDs that have this weird over-exposed look to them
	(Sonic Secret Rings and Sonic Heroes Xbox for example).
	
	Uses raw YUV420 video, you can convert with FFmpeg like this:
	
		ffmpeg -i video.sfd video.yuv
	
	Then run program like this
	
		csc_video_conv video.yuv original.sfd
		
	It will create a file which is raw YUV420p video that you can use.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plane_converters.h>
#include <pathutils.h>
#include <mpeg.h>
#include <m1v.h>
#include <sfd.h>

uint8_t check_args(int argc, char** argv)
{
	uint8_t ret = 0;
	
	if(argc != 3)
	{
		printf("Incorrect parameters.");
		return 1;
	}
	
	FILE* yuv_file = fopen(argv[1], "rb");
	FILE* sfd_file = fopen(argv[2], "rb");
	
	if(yuv_file == NULL)
	{
		printf("Problem opening the YUV file\n");
		ret = 1;
	}
	
	if(sfd_file == NULL)
	{
		printf("Problem opening the SFD file\n");
		ret = 1;
	}

	fclose(yuv_file);
	fclose(sfd_file);
	return ret;
}

int main(int argc, char** argv)
{
	const uint8_t stat = check_args(argc, argv);
	
	if(stat == 1)
	{
		return 0;
	}
	
	FILE* yuv_file = fopen(argv[1], "rb");
	fseek(yuv_file, 0, SEEK_END);
	const uint64_t yuv_file_size = ftell(yuv_file);
	fseek(yuv_file, 0, SEEK_SET);
	
	FILE* sfd_file = fopen(argv[2], "rb");
	fseek(sfd_file, 0, SEEK_END);
	const uint64_t sfd_file_size = ftell(sfd_file);
	fseek(sfd_file, 0, SEEK_SET);
	
	mpeg_frame_s mpeg_frame = {0};
	M1v_info m1v_info = {0};
	
	m1v_info.file_buffer = (uint8_t*)calloc(sfd_file_size, 1);

	// Let's get raw m1v video from sfd

	while(1)
	{
		if(ftell(sfd_file) == sfd_file_size) break;
		
		mpeg_get_next_frame(sfd_file, &mpeg_frame);
	
		if(mpeg_frame.stream == STREAM_VIDEO)
		{
			if(mpeg_frame.sync == MPEG_VIDEO_START_ID)
			{
				memcpy(&m1v_info.file_buffer[m1v_info.file_size], mpeg_frame.data, mpeg_frame.len);
				m1v_info.file_size += mpeg_frame.len;
			}
		}
	}
	
	fclose(sfd_file);
	
	// Now getting video info
	m1v_next_packet(&m1v_info);
	
	// Preparing output file name
	File_path_s rgb_path;
	split_path(argv[1], 0, &rgb_path);
	
	mut_cstring conv = {0};
	conv.ptr = (char*)malloc(11);
	strncpy(conv.ptr, "_converted", 10);
	conv.size = 10;
	add_to_string(&rgb_path.file_name, &conv);
	
	printf("%s\n", create_string(&rgb_path));
	
	FILE* rgb_file = fopen(create_string(&rgb_path), "wb");
	const uint32_t color_ch_size = (m1v_info.width*m1v_info.height);
	const uint32_t bytes_per_frame = color_ch_size + color_ch_size/2;
	const uint32_t bytes_per_frame_rgb = color_ch_size*3;
	const uint32_t frames_total = yuv_file_size/bytes_per_frame;
	uint8_t* data_buffer = (uint8_t*)malloc(bytes_per_frame);
	uint8_t* rgb_buffer = (uint8_t*)malloc(bytes_per_frame_rgb);
	
	printf("WIDTH:   %u\n", m1v_info.width);
	printf("HEIGHT:  %u\n", m1v_info.height);
	printf("BPF:     %u\n", bytes_per_frame);
	printf("BPF RGB: %u\n", bytes_per_frame_rgb);
	printf("FRAMES:  %u\n", frames_total);
	
	uint8_t* frame_yuv_types = (uint8_t*)malloc(frames_total);
	memset(frame_yuv_types, 'N', frames_total); // Defaults to 'N' type
	
	uint32_t bytes_read = 0;
	uint32_t it = 0;
	while(bytes_read = m1v_next_packet(&m1v_info))
	{
		if(m1v_info.last_stream_id == 0xB2)
		{
			const uint32_t pos = m1v_info.file_pos-bytes_read+4;
			const uint8_t* str = &m1v_info.file_buffer[pos];
			
			if(strncmp("<SUDPS_>", &str[0], 8) == 0)
			{
				frame_yuv_types[it++] = str[19];
			}
		}
	}
	
	yuv_s yuv;
	memset(&yuv, 0, sizeof(yuv_s));
	
	init_YUV420(&yuv, m1v_info.width, m1v_info.height);
	
	it = 0;
	while(fread(data_buffer, 1, bytes_per_frame, yuv_file) != 0)
	{
		load_raw_YUV420(&yuv, data_buffer);
		
		if(frame_yuv_types[it] == 'C')
		{
			for(uint32_t i = 0; i != yuv.y.w*yuv.y.h; ++i)
			{
				yuv.y.data[i] = sfd_csc_to_raw_y(yuv.y.data[i]);
			}

			for(uint32_t i = 0; i != yuv.u.w*yuv.u.h; ++i)
			{
				yuv.u.data[i] = sfd_csc_to_raw_uv(yuv.u.data[i]);
				yuv.v.data[i] = sfd_csc_to_raw_uv(yuv.v.data[i]);
			}
		}
		
		/*conv_YUV420_plane_to_RGB(m1v_info.width, m1v_info.height,
								 yuv.y.data, yuv.v.data,
								 yuv.u.data, rgb_buffer);*/
								 
		//fwrite(rgb_buffer, bytes_per_frame_rgb, 1, rgb_file);
		
		fwrite(yuv.y.data, color_ch_size, 1, rgb_file);
		fwrite(yuv.u.data, color_ch_size/4, 1, rgb_file);
		fwrite(yuv.v.data, color_ch_size/4, 1, rgb_file);
		
		printf("[%05u/%05u]\r", it++, frames_total);
	}
	
	destroy_YUV_frame(&yuv);
	free(data_buffer);
	free(rgb_buffer);
	free(frame_yuv_types);
	fclose(yuv_file);
	
	return 0;
	
}