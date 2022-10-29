#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <mpeg.h>
#include <sfd.h>
#include <common.h>

/*
	Functions
*/

FILE* test_args(int argc, char** argv);

sfd_streams_u get_sfd_info(FILE* sfdf, mpeg_frame_s* mpeg, uint8_t* stream_ver);

/*
	Entry point
*/
int main(int argc, char** argv)
{	
	FILE* sfdf = test_args(argc, argv);
	if(!sfdf) return 0;

	uint8_t stream_ver = 0;
	mpeg_frame_s mpeg_frame;
	sfd_streams_u sfd_stream = get_sfd_info(sfdf, &mpeg_frame, &stream_ver);
	
	fclose(sfdf);
	
	return 0;
}

FILE* test_args(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Usage: sfdinfo file.sfd\n");
		return 0;
	}
	
	FILE* sfdf = fopen(argv[1], "rb");
	
	if(!sfdf)
	{
		printf("Couldn't open a file\n");
		return 0;
	}
	
	return sfdf;
}

sfd_streams_u get_sfd_info(FILE* sfdf, mpeg_frame_s* mpeg, uint8_t* stream_ver)
{
	sfd_streams_u str = {0};
	*stream_ver = 0;
	
	while(1)
	{
		if(*stream_ver != 0)
			break;
		
		mpeg_get_next_frame(sfdf, mpeg);
		
		const uint32_t c = fgetc(sfdf);
		if(c == EOF) break;
		fseek(sfdf, -1, SEEK_CUR);
		
		if(mpeg->sync == PS_PRIV2)
		{
			memcpy(&str.s1, &mpeg->data[6], sizeof(sfd_streams_u));
			
			if(strncmp("SofdecStream", str.s1.sofdec_string, 12) == 0)
			{				
				if(str.s2.sofdec_string[12] == '2')
					*stream_ver = 2;
				else
					*stream_ver = 1;
			
				printf("===SofdecStream\n");
				printf("Version: %u\n", *stream_ver);
				
				switch(*stream_ver)
				{
					case 1:
						printf("unk_0x26: %x\n", str.s1.unk_0x26);
						printf("unk_0x27: %x\n", str.s1.unk_0x27);
						printf("MPEG Pack Size: %u\n", str.s1.mpeg_pack_size);
						printf("unk_0x70: %x\n", str.s1.unk_0x70);
						printf("unk_0x76: %x\n", str.s1.unk_0x76);
						printf("mpeg_pack_size_again: %u\n", str.s1.mpeg_pack_size_again);
						printf("total_streams: %u\n", str.s1.total_streams);
						printf("audio_streams: %u\n", str.s1.audio_streams);
						printf("video_streams: %u\n", str.s1.video_streams);
						printf("private_streams: %u\n", str.s1.private_streams);
						printf("unk_0xA2: %x\n", str.s1.unk_0xA2);
						printf("longest_audio_len: %u\n", str.s1.longest_audio_len);
						printf("longest_video_len: %u\n", str.s1.longest_video_len);
						printf("max_num_video_frames: %u\n", str.s1.max_num_video_frames);
						printf("max_size_pictures: %u\n", str.s1.max_size_pictures);
						printf("avg_size_pictures: %u\n", str.s1.avg_size_pictures);
						printf("User comment: %s\n", str.s1.user_comment_0xCE);
				
						for(uint32_t i = 0; i != str.s1.total_streams-1; ++i)
						{
							printf("====STREAM [%u]:\n", i+1);
							printf("out_name: % .8s\n", str.s1.stream_entries[i].audio.out_name);
							printf("out_ext: % .4s\n", str.s1.stream_entries[i].audio.out_ext);
							printf("out_date: % .12s\n", str.s1.stream_entries[i].audio.out_date);
							printf("stream_id: %02x\n", str.s1.stream_entries[i].audio.stream_id);

							const uint8_t is_audio = str.s1.stream_entries[i].audio.stream_id >= 0xE0 ? 0 : 1;

							if(is_audio) // is audio
							{
								printf("channels: %u\n", str.s1.stream_entries[i].audio.channels);
								printf("sample_rate: %u\n", str.s1.stream_entries[i].audio.sample_rate);
							}
							else if(is_audio == 0)
							{
								printf("width: %u\n", str.s1.stream_entries[i].video.width<<4);
								printf("height: %u\n", change_order_16(str.s1.stream_entries[i].video.height));
							}
						}
						break;
					case 2:
						printf("mpeg_pack_size: %u\n", change_order_16(str.s2.mpeg_pack_size));
						printf("unk_0x26: %x\n", str.s2.unk_0x26);
						printf("unk_0x27: %x\n", str.s2.unk_0x27);
						printf("unk_0x28: %x\n", str.s2.unk_0x28);
						printf("unk_0x29: %x\n", str.s2.unk_0x29);
						printf("muxer_name: %.32s\n", str.s2.muxer_name);
						printf("total_streams: %u\n", str.s2.total_streams);
						printf("audio_streams: %u\n", str.s2.audio_streams);
						printf("video_streams: %u\n", str.s2.video_streams);
						printf("private_streams: %u\n", str.s2.private_streams);
						printf("unk_0xB2: %x\n", str.s2.unk_0xC2);
						printf("unk_0xC6: %x\n", str.s2.unk_0xC6);
						printf("unk_0xCA: %x\n", str.s2.unk_0xCA);
						
						for(uint32_t i = 0; i != str.s2.audio_streams; ++i)
						{
							printf("====AUDIO STREAM [%u]:\n", i+1);
							printf("stream_id: %x\n", str.s2.audio_entries[i].stream_id);
							printf("audio_format: %u", str.s2.audio_entries[i].audio_format);
							printf(" (%s)\n", sfd_audio_formats_2[str.s2.audio_entries[i].audio_format]);
							printf("unk_0x01_0: %x\n", str.s2.audio_entries[i].unk_0x01_0);
							printf("unk_0x02_4: %x\n", str.s2.audio_entries[i].unk_0x02_4);
							printf("channels: %u\n", str.s2.audio_entries[i].channels);
							printf("sample_rate: %u\n", change_order_16(str.s2.audio_entries[i].sample_rate));
							printf("rate_x_duration: %u\n", change_order_32(str.s2.audio_entries[i].rate_x_duration));
							printf("unk_0x09: %x\n", str.s2.audio_entries[i].unk_0x09);
							printf("unk_0x0A: %x\n", str.s2.audio_entries[i].unk_0x0A);
							printf("unk_0x0B: %x\n", str.s2.audio_entries[i].unk_0x0B);
							printf("unk_0x0D: %x\n", str.s2.audio_entries[i].unk_0x0D);
							printf("unk_0x0E: %x\n", str.s2.audio_entries[i].unk_0x0E);
						}
						
						for(uint32_t i = 0; i != str.s2.video_streams; ++i)
						{
							printf("====VIDEO STREAM [%u]:\n", i+1);
							printf("stream_id: %x\n", str.s2.video_entries[i].stream_id);
							printf("unk_0x01: %x\n", str.s2.video_entries[i].unk_0x01);
							printf("width: %u\n", change_order_16(str.s2.video_entries[i].width));
							printf("height: %u\n", change_order_16(str.s2.video_entries[i].height));
							printf("frames: %u\n", change_order_32(str.s2.video_entries[i].frames));
							printf("framerate: %u\n", change_order_16(str.s2.video_entries[i].framerate));
							printf("unk_0x0C: %x\n", str.s2.video_entries[i].unk_0x0C);
							printf("unk_0x10: %x\n", str.s2.video_entries[i].unk_0x10);
							printf("unk_0x20: %x\n", str.s2.video_entries[i].unk_0x20);
							printf("unk_0x22: %x\n", str.s2.video_entries[i].unk_0x22);
							printf("unk_0x24: %x\n", str.s2.video_entries[i].unk_0x24);
							printf("unk_0x26: %x\n", str.s2.video_entries[i].unk_0x26);
							printf("unk_0x28: %x\n", str.s2.video_entries[i].unk_0x28);
							printf("unk_0x2A: %x\n", str.s2.video_entries[i].unk_0x2A);
						}
						
						printf("unk_0x7CE: %x\n", str.s2.unk_0x7CE);
						break;
				}
			}
				
			break;
		}
	}
	
	return str;
}