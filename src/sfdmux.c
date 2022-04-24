#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <adx.h>
#include <mpeg.h>
#include <utils.h>
#include <sfd.h>
#include <io_common.h>
#include <common.h>

/*
	Functions used to keep files in check.
*/

File_desc* prepare_files(int argc, char** argv, uint8_t* files_amount);
void destroy_file_desc(File_desc* files, const uint8_t files_amount);

/*
	Gives you the amount of frames ADX file will have and the SCR time step.
*/
void get_adx_frame_info(const ADX* adx, uint32_t* frame_count, float* time_per_frame);

/*
	Prepares ADX mpeg frame to write
*/
uint8_t* prepare_adx_mpeg_frame(FILE* adx_file, const float adx_global_scr, const uint32_t id, uint32_t* data_size);

/*
	Muxes video and audio
*/
void mux(File_desc* files, const uint8_t files_amount);

/*
	Entry point
*/
int main(int argc, char** argv)
{	
	if(argc <= 2)
	{
		printf("Read README-sfdmux for usage.\n");
		return 0;
	}

	uint8_t files_amount = 0;
	File_desc* files_ptr = prepare_files(argc, argv, &files_amount);
	
	printf("\nAmount of files: %u\n", files_amount);
	
	mux(files_ptr, files_amount);
	
	destroy_file_desc(files_ptr, files_amount);
}

void get_adx_frame_info(const ADX* adx, uint32_t* frame_count, float* time_per_frame)
{
	//*frame_count = adx->file_size/(MPEG_MAX_DATA_SIZE-7);
	const uint16_t adx_frame_size_full = 2035 + 7;
	*frame_count = adx->file_size/adx_frame_size_full;
	
	if(adx->file_size%adx_frame_size_full != 0)
	{
		*frame_count += 1;
	}
	
	*time_per_frame = adx->est_length/(float)(*frame_count);
}

uint8_t* prepare_adx_mpeg_frame(FILE* adx_file, const float adx_global_scr, const uint32_t id, uint32_t* data_size)
{
	uint8_t* data = (uint8_t*)calloc(1, MPEG_MAX_DATA_SIZE+1);
	
	const uint32_t file_pos_before = ftell(adx_file);
	const uint32_t stream_id = change_order_32(id);
	const uint64_t scr_to_encode = adx_global_scr*MPEG_SCR_MUL;
	uint8_t scr[5] = {0};
	mpeg1_encode_scr(scr, scr_to_encode);

	memcpy(&data[0], &stream_id, 4);
	memcpy(&data[6], &adx_frame_value, 2);
	memcpy(&data[8], scr, 5);
	
	// 2035 because all metadata and to fit into 0x800 boundary
	fread(&data[13], 2035, 1, adx_file);
	const uint32_t file_pos_after = ftell(adx_file);
	const uint16_t bytes_read = file_pos_after - file_pos_before;
	
	// +7 because 0x4004 and 5bytes of SCR
	const uint16_t bytes_read_BE = change_order_16(bytes_read+7);
	
	memcpy(&data[4], &bytes_read_BE, 2);

	//  13 being metadata
	*data_size = bytes_read + 13;

	return data;
}

void mux(File_desc* files, const uint8_t files_amount)
{
	//FILE* log = fopen("log.txt", "wb");
	struct Mpeg1Frame mpeg_frame;
	memset(&mpeg_frame, 0, sizeof(struct Mpeg1Frame));
	
	uint8_t adx_done = 0;
	float mpeg_global_scr = 0;

	// Sonic Unleashed weird hack
	mpeg1_write_padding(files[0].f, 2042);

	/* SFD header and metadata */
	printf("Writing SFD header\n");
	sfd_sofdec2_mpeg_packet(files, files_amount);
	
	/* Audio must be in the future i guess? */
	for(uint8_t i = 2; i != files_amount; ++i)
	{
		files[i].adx_cur_scr = files[i].adx_scr_step*4;
	}
	
	while(1)
	{
		if(files[1].file_done == 0)
		{
			mpeg_get_next_frame(files[1].f, &mpeg_frame);
			mpeg_global_scr = mpeg_frame.last_scr/(float)MPEG_SCR_MUL;
			//fprintf(log, "mpeg scr: %f\n", mpeg_global_scr);
			
			if(mpeg_frame.stream == STREAM_VIDEO)
			{
				fwrite(mpeg_frame.data, mpeg_frame.len, 1, files[0].f);
				const uint16_t dvd_boundary = 2048 - mpeg_frame.len;
				
				if(dvd_boundary != 0)
				{
					mpeg1_write_padding(files[0].f, dvd_boundary - 6);
				}
			}
			
			if(ftell(files[1].f) == files[1].file_size)
			{
				files[1].file_done = 1;
			}
		}
		
		for(uint8_t i = 2; i != files_amount; ++i)
		{
			if(files[i].file_done == 0)
			{
				if(files[i].adx_cur_scr <= mpeg_global_scr || files[1].file_done == 1)
				{
					uint32_t data_size;
					const uint8_t* data_to_write = prepare_adx_mpeg_frame(files[i].f, files[i].adx_cur_scr, files[i].stream_id, &data_size);
					fwrite(data_to_write, data_size, 1, files[0].f);
					
					files[i].adx_frame_count -= 1;
					
					if(files[i].adx_frame_count == 0)
					{
						files[i].file_done = 1;
						adx_done += 1;
						printf("ADX Done: %u\n", adx_done);
					}
				
					files[i].adx_cur_scr += files[i].adx_scr_step;
								
					//fprintf(log, "adx %u scr: %f\n", i-2, files[i].adx_cur_scr);
				}
			}
		}
		
		if(adx_done == (files_amount-2))
		{
			break;
		}
	}
}


File_desc* prepare_files(int argc, char** argv, uint8_t* files_amount)
{
	File_desc* files_ptr = calloc(MPEG_MAX_AUDIO_STREAMS+2, sizeof(File_desc));
	memset(files_ptr, 0, sizeof(File_desc)*(MPEG_MAX_AUDIO_STREAMS+2));
	*files_amount = 0;
	
	uint8_t audio_files_amount = 0;
	FILE* f;
	ADX adx;
	
	for(uint8_t i = 1; i != argc; ++i)
	{
		printf("[%u/%u] %s\n", i, argc-1, argv[i]);
		
		/* First file is output */
		if(i == 1)
		{
			printf("\tThis is an output file.\n");

			files_ptr[0].f = fopen(argv[i], "wb");
			files_ptr[0].is_output = 1;
			
			*files_amount += 1;

			continue;
		}
		
		/* 
			Second file is an mpeg file with all of its streams.
			All audio streams in it will be ignored.
		*/
		if(i == 2)
		{
			printf("\tThis is an MPEG file.\n");

			files_ptr[1].f = fopen(argv[i], "rb");
			files_ptr[1].is_mpeg = 1;
			
			fseek(files_ptr[1].f, 0, SEEK_END);
			files_ptr[1].file_size = ftell(files_ptr[1].f);
			rewind(files_ptr[1].f);
			
			*files_amount += 1;

			continue;
		}
		
		/* Checking if it is an ADX or AIX file */
		
		if((audio_files_amount) == MPEG_MAX_AUDIO_STREAMS)
		{
			printf("\tMaximum audio files reached.\n");
			break;
		}
		
		f = fopen(argv[i], "rb");
		adx = read_adx_info(f);
		fclose(f);
		
		if(adx.info.magic != 0xFFFF)
		{
			audio_files_amount += 1;
			*files_amount += 1;
			
			const uint8_t cur_idx = audio_files_amount+1; 
			
			if(adx.info.magic == 0x8000)
			{
				printf("\tThis is an ADX file.\n");
				files_ptr[cur_idx].is_adx = 1;
			}
			if(adx.info.magic == 0x4149)
			{
				printf("\tThis is an AIX file.\n");
				files_ptr[cur_idx].is_aix = 1;
			}
			
			files_ptr[cur_idx].stream_id = MPEG_AUDIO_START_ID+(audio_files_amount-1);
			
			files_ptr[cur_idx].f = fopen(argv[i], "rb");
			
			files_ptr[cur_idx].file_size = adx.file_size;
			files_ptr[cur_idx].channel_count = adx.info.channel_count;
			files_ptr[cur_idx].sample_rate = adx.info.sample_rate;
			get_adx_frame_info(&adx, &files_ptr[cur_idx].adx_frame_count, &files_ptr[cur_idx].adx_scr_step);
			
			printf("\tIdx: %u\n", cur_idx);
			printf("\tSize: %u\n", files_ptr[cur_idx].file_size);
			printf("\tFrames: %u\n", files_ptr[cur_idx].adx_frame_count);
			printf("\tStep: %f\n", files_ptr[cur_idx].adx_scr_step);
			printf("\tID: %08x\n", files_ptr[cur_idx].stream_id);
			
			continue;
		}
		
		// If everything failed
		printf("\tThis is not an ADX/AIX file.\n");
	}
	
	return files_ptr;
}

void destroy_file_desc(File_desc* files, const uint8_t files_amount)
{
	for(uint8_t i = 0; i != files_amount; ++i)
	{
		fclose(files[i].f);
	}
	
	free(files);
}