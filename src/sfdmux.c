#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <adx.h>
#include <m1v.h>
#include <mpeg.h>
#include <sfd.h>
#include <io_common.h>
#include <common.h>
#include <fd.h>


/*
	Globals are never okay, remember kids
*/
uint8_t video_files_amount = 0;
uint8_t audio_files_amount = 0;
static sfd_frame_metadata_s sfd_metadata;

double mpeg_global_scr = 0;
static uint16_t dvd_boundary = MPEG_DVD_BLOCK;

static uint8_t packet_buffer[MPEG_DVD_BLOCK] = {0};

/*
	Gives you the amount of frames a file will have and the SCR time step.
*/
void get_frame_info(File_desc* fd, uint32_t* frame_count, double* time_per_frame);

/*
	Prepares ADX/M1V mpeg frame to write
*/
void prepare_mpeg_frame(File_desc* file, uint8_t* buffer, uint16_t* data_size, const uint16_t available);

/*
	Writes initial packets of MPEG-PS for SFD.
	
*/
void write_initial_packets(File_desc* files, const uint8_t files_amount);

/*
	Muxes video and audio
*/
void mux(File_desc* files, const uint8_t files_amount);


/*
	Function used to keep files in check.
*/

File_desc* prepare_files(int argc, char** argv, uint8_t* files_amount);

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

	// Identity from RE: Darkside Chronicles
	// 3C 53 55 44 50 53 5F 3E 3C 30 30 30 30 30 36 3E 30 32 41 43 50
	// <SUDPS_><000006>02ACP
	const uint8_t fake_identity[] = "<SUDPS_><000006>02ACP";
	const uint8_t sbk_identity[] =  "<SUDPS_><000006>02ANP";
	sfd_metadata.data = (uint8_t*)malloc(0x15);
	memcpy(sfd_metadata.data, sbk_identity, 0x15);
	sfd_metadata.size = 0x15;

	uint8_t files_amount = 0;
	File_desc* files_ptr = prepare_files(argc, argv, &files_amount);
	printf("\nAmount of files: %u\n", files_amount);
	
	write_initial_packets(files_ptr, files_amount);
	
	mux(files_ptr, files_amount);
	
	mpeg_write_prog_end(files_ptr[0].f);
	
	file_desc_destroy(files_ptr, files_amount);
	
	printf("Muxing done.\n");
}

void get_frame_info(File_desc* fd, uint32_t* frame_count, double* time_per_frame)
{
	// We're subtracting Pack header size and
	// non-payload stream stuff like header, size and 0x0F (no extension)
	uint16_t frame_size = MPEG_DVD_BLOCK - 12 - 4 - 2 - 1;
	uint64_t m1v_size = 0;
	
	switch(fd->file_type)
	{
		case FD_ADX:
		case FD_AIX:
			// for ADX, extension is fixed size, so we add 11 bytes more
			frame_size += 11;
			*frame_count = fd->file_size/frame_size;
			*time_per_frame = fd->data.adx.est_length/(double)(*frame_count);
			break;
		case FD_M1V:
			m1v_size = fd->file_size;
			
			// We need to add user data which contains SFD Identity
			if(sfd_metadata.data != 0)
			{
				m1v_size += sfd_metadata.size*fd->data.m1v.cur_frame;
			}
		
			*frame_count = m1v_size/frame_size;
			*time_per_frame = fd->data.m1v.time_as_double/(double)(*frame_count);
			break;
	}
	
	fd->first_frame = 1;
	fd->frame_count_max = (*frame_count);
}

void write_initial_packets(File_desc* files, const uint8_t files_amount)
{
	const uint64_t scr_to_encode = mpeg_global_scr*MPEG_SCR_MUL;
	
	mpeg_write_pack_header(files[0].f, scr_to_encode, 0);
	dvd_boundary -= 0xC;
	dvd_boundary -= mpeg_write_system_header(files[0].f, audio_files_amount, 0);
	mpeg_write_padding(files[0].f, dvd_boundary);
	
	dvd_boundary = MPEG_DVD_BLOCK;
	
	mpeg_write_pack_header(files[0].f, scr_to_encode, 0);
	dvd_boundary -= 0xC;
	dvd_boundary -= mpeg_write_system_header(files[0].f, 0, video_files_amount);
	mpeg_write_padding(files[0].f, dvd_boundary);
	
	// SofdecStream
	mpeg_write_pack_header(files[0].f, scr_to_encode, 0);
	sfd_sofdec2_mpeg_packet(files, files_amount);
	
	// Metadata after sofdec packet, can be anything i think
	// Never mind it CANNOT BE ANYTHING
	// Sonic Black Knight won't even play the video with
	// this "silly comment!" thing
	// WHAT EVEN IS THIS FORMAT, CRIWARE
	/*dvd_boundary = MPEG_DVD_BLOCK;
	mpeg_write_pack_header(files[0].f, scr_to_encode, 0);
	dvd_boundary -= 0xC;
	
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint8_t message[] = "Silly comment!";
	const uint16_t data_size = change_order_16(sizeof(message)-1);
	fwrite(&mpeg_ps_priv2, sizeof(uint32_t), 1, files[0].f);
	fwrite(&data_size, sizeof(uint16_t), 1, files[0].f);
	fwrite(&message, sizeof(message)-1, 1, files[0].f);
	dvd_boundary -= 0x14;*/
	
	//mpeg_write_padding(files[0].f, dvd_boundary);
}

void prepare_mpeg_frame(File_desc* file, uint8_t* buffer, uint16_t* data_size, const uint16_t available)
{
	uint16_t remaining = 0;
	
	if((file->file_type == FD_ADX) || (file->file_type == FD_AIX))
	{
		const uint64_t available_in_adx = file->file_size - file->file_pos;
		uint8_t* data = &file->file_buffer[file->file_pos];

		const uint16_t ret = mpeg_prep_stream_packet(buffer, file->stream_id,
													 data, available_in_adx, available,
													 1, 4,
													 file->cur_scr*MPEG_SCR_MUL, file->cur_scr*MPEG_SCR_MUL);
		
		file->file_pos += ret;
		*data_size = available;
	}
	else if(file->file_type == FD_M1V)
	{
		const uint64_t available_in_m1v = file->file_size - file->file_pos;
		uint8_t* data = &file->file_buffer[file->file_pos];

		uint64_t which_frame = 0;
		/*for(uint64_t i = 0; i != file->data.m1v.cur_frame; ++i)
		{
			if(file->data.m1v.frames_positions[i] < file->file_pos) break;
			which_frame = i;
			//printf("[%u] | previous frame %u | which frame: %u\n", file->data.m1v.cur_frame, file->previous_frame, which_frame);
		}
		
		if(which_frame == file->previous_frame) which_frame = -1;*/

		uint8_t buf_scale = 1;
		uint16_t buf_size = 46;
		const double fps_frac = 1.f/mpeg1_framerate[file->data.m1v.frame_rate];
		uint64_t pts = file->cur_scr*MPEG_SCR_MUL+fps_frac;
		uint64_t dts = file->cur_scr*MPEG_SCR_MUL;
		
		if(file->first_frame == 1)
		{
			pts = 1;
			dts = 0;
			file->first_frame = 0;
		}
		else if(available_in_m1v > available)
		{
			// Otherwise, don't write the extension
			buf_size = 0;
			buf_scale = 0; 
		}
		
		if(file->scr_counter > 5.f)
		{
			file->scr_counter = 0.f;
			buf_scale = 1;
			buf_size = 46;
			pts = file->cur_scr*MPEG_SCR_MUL;
			dts = file->cur_scr*MPEG_SCR_MUL-1024;
		}
		
		const uint16_t ret = mpeg_prep_stream_packet(buffer, file->stream_id,
													 data, available_in_m1v, available,
													 buf_scale, buf_size,
													 pts, dts);

		file->file_pos += ret;
		*data_size = available;
		file->previous_frame = which_frame;
	}
}

void mux(File_desc* files, const uint8_t files_amount)
{
	uint8_t files_done = 0;
	dvd_boundary = MPEG_DVD_BLOCK;
	
	while(1)
	{		
		for(uint8_t i = 1; i != files_amount; ++i)
		{
			if(files[i].file_done == 0)
			{
				if(files[i].cur_scr <= mpeg_global_scr)
				{
					const uint64_t scr_to_encode = mpeg_global_scr*MPEG_SCR_MUL;
					mpeg_write_pack_header(files[0].f, scr_to_encode, 0);
					dvd_boundary -= 0xC;
					
					uint16_t data_size = 0;
					
					prepare_mpeg_frame(&files[i], &packet_buffer[0], &data_size, dvd_boundary);
					fwrite(packet_buffer, data_size, 1, files[0].f);
					
					if(files[i].file_size == files[i].file_pos)
					{
						files[i].file_done = 1;
						files_done += 1;
						printf("File done: %u\n", files_done);
						printf("%lf\n", files[i].cur_scr);
					}
				
					files[i].cur_scr += files[i].scr_step;
					files[i].scr_counter += files[i].scr_step;
					dvd_boundary = MPEG_DVD_BLOCK;
				}
			}
		}
		
		mpeg_global_scr += 0.0001f;
		
		if(files_done == (files_amount-1))
		{
			break;
		}
	}
}

File_desc* prepare_files(int argc, char** argv, uint8_t* files_amount)
{
	const uint8_t files_descs_available = MPEG_MAX_AUDIO_STREAMS+MPEG_MAX_VIDEO_STREAMS+1;
	File_desc* files_ptr = calloc(files_descs_available, sizeof(File_desc));
	memset(files_ptr, 0, sizeof(File_desc)*files_descs_available);
	
	*files_amount = 0;
	
	for(uint8_t i = 1; i != argc; ++i)
	{
		printf("[%u/%u] %s\n", i, argc-1, argv[i]);
		
		/* First file is output */
		if(i == 1)
		{
			printf("\tThis is an output file.\n");

			files_ptr[0].f = fopen(argv[i], "wb");
			split_path(argv[i], 0, &files_ptr[0].f_path);
			files_ptr[0].file_type = FD_OUTPUT;
			
			*files_amount += 1;

			continue;
		}
		
		const uint8_t cur_idx = i - 1;
		
		/* Checking if it is an ADX or AIX */
		
		if((audio_files_amount) == MPEG_MAX_AUDIO_STREAMS)
		{
			printf("\tMaximum audio files reached.\n");
			continue;
		}
		
		const uint8_t adx_status = adx_check_file(argv[i]);
		
		if(adx_status)
		{	
			audio_files_amount += 1;
			*files_amount += 1;

			files_ptr[cur_idx].data.adx = adx_read_info(argv[i]);
			files_ptr[cur_idx].stream_id = MPEG_AUDIO_START_ID+(audio_files_amount-1);
			files_ptr[cur_idx].f = fopen(argv[i], "rb");
			split_path(argv[i], 0, &files_ptr[cur_idx].f_path);
			
			switch(files_ptr[cur_idx].data.adx.info.magic)
			{
				case 0x8000:
					printf("\tThis is an ADX file.\n");
					files_ptr[cur_idx].file_type = FD_ADX;
					break;
				case 0x4149:
					printf("\tThis is an ADX file.\n");
					files_ptr[cur_idx].file_type = FD_AIX;
					break;
				
				default:
					printf("Somehow, unknown audio type.\n");
					continue;
			}
			
			files_ptr[cur_idx].file_size = files_ptr[cur_idx].data.adx.file_size;
			files_ptr[cur_idx].file_pos = files_ptr[cur_idx].data.adx.file_pos;
			files_ptr[cur_idx].file_buffer = files_ptr[cur_idx].data.adx.file_buffer;
			
			get_frame_info(&files_ptr[cur_idx], &files_ptr[cur_idx].frame_count, &files_ptr[cur_idx].scr_step);
			
			printf("\tLength: %lf\n", files_ptr[cur_idx].data.adx.est_length);
			
			printf("\tSize: %u\n", files_ptr[cur_idx].data.adx.file_size);
			printf("\tStep: %.15lf\n", files_ptr[cur_idx].scr_step);
			printf("\tID: %08x\n", files_ptr[cur_idx].stream_id);
			
			continue;
		}
		
		/* Checking if it is an M1V or M2V */

		if((video_files_amount) == MPEG_MAX_VIDEO_STREAMS)
		{
			printf("\tMaximum video files reached.\n");
			continue;
		}

		M1v_info m1v = m1v_test_file(argv[i]);

		if(m1v.codec != 0)
		{
			video_files_amount += 1;
			*files_amount += 1;
			
			split_path(argv[i], 0, &files_ptr[cur_idx].f_path);
			files_ptr[cur_idx].file_type = FD_M1V;
			files_ptr[cur_idx].stream_id = MPEG_VIDEO_START_ID+(video_files_amount-1);
			
			m1v_init(argv[i], &files_ptr[cur_idx].data.m1v);
			files_ptr[cur_idx].data.m1v.time_as_double = m1v.time_as_double; // to calc SCR step
			files_ptr[cur_idx].data.m1v.cur_frame = m1v.cur_frame; // to calc SCR step
			files_ptr[cur_idx].data.m1v.frames_positions = m1v.frames_positions;
			m1v.frames_positions = NULL;
			files_ptr[cur_idx].data.m1v.width = m1v.width;
			files_ptr[cur_idx].data.m1v.height = m1v.height;
			
			sfd_m1v_insert_userdata(&sfd_metadata, &files_ptr[cur_idx].data.m1v);
		
			files_ptr[cur_idx].file_size = files_ptr[cur_idx].data.m1v.file_size;
			files_ptr[cur_idx].file_pos = files_ptr[cur_idx].data.m1v.file_pos;
			files_ptr[cur_idx].file_buffer = files_ptr[cur_idx].data.m1v.file_buffer;
			
			get_frame_info(&files_ptr[cur_idx], &files_ptr[cur_idx].frame_count, &files_ptr[cur_idx].scr_step);
			
			printf("\tThis is an M%uV file.\n", m1v.codec);
			
			printf("\tResolution: %hux%hu\n", m1v.width, m1v.height);
			printf("\tFramerate: %.3lf\n", mpeg1_framerate[m1v.frame_rate]);
			printf("\tLength: %02u:%02u:%02u.%03u\n", m1v.time_hour, m1v.time_minute, m1v.time_second, m1v.time_milliseconds);
			printf("\tLength as double: %lf\n", m1v.time_as_double);
			printf("\tFrames: %u\n", files_ptr[cur_idx].data.m1v.cur_frame);
			printf("\tLast frame: 0x%x\n", files_ptr[cur_idx].data.m1v.frames_positions[m1v.cur_frame-1]);
			
			printf("\tSize: %u\n", files_ptr[cur_idx].data.m1v.file_size);
			printf("\tStep: %.15lf\n", files_ptr[cur_idx].scr_step);
			printf("\tID: %08x\n", files_ptr[cur_idx].stream_id);
			
			m1v_destroy(&m1v);
			
			continue;
		}
		
		// If everything failed
		printf("\tThis is not a valid file.\n");
	}
	
	return files_ptr;
}