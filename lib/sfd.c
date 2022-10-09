#include "sfd.h"

const static uint8_t sofdec_header[] = "SofdecStream            ";
const static uint8_t sofdec2_header[] = "SofdecStream2           ";
const uint8_t sfdmux_info[] = "Created using sfdmux https://github.com/ThisKwasior/CryTools";
const uint8_t multiplexer[] = "sfdmux by Kwasior";

const static uint8_t sofdec_comment[64] = "Created using sfdmux https://github.com/ThisKwasior/CryTools    ";

void sfd_sofdec_mpeg_packet(File_desc* fd, const uint8_t fd_size)
{
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint16_t data_size = 60935; // 2030 LE
	fwrite(&mpeg_ps_priv2, sizeof(uint32_t), 1, fd[0].f);
	fwrite(&data_size, sizeof(uint16_t), 1, fd[0].f);
	
	sfd_stream_s sfd;
	memset(&sfd, 0, sizeof(sfd_stream_s));
	
	// sfd init
	memcpy(sfd.sofdec_string, sofdec_header, sizeof(sofdec_header));
	sfd.unk1 = 0x02; 
	sfd.unk2 = 0x1C;
	
	memset(sfd.out_file.video.out_name, 0x20, 8);
	memcpy(sfd.out_file.video.out_name,
		   fd[0].f_path.file_name.ptr,
		   fd[0].f_path.file_name.size >= 8 ? 8 : fd[0].f_path.file_name.size);
		   
	const uint8_t sfd_ext[] = ".sfd";
	const uint8_t sfd_date[] = "111122334455";
	memcpy(sfd.out_file.video.out_ext, sfd_ext, 4);
	memcpy(sfd.out_file.video.out_date, sfd_date, 12);
	memcpy(sfd.out_file.video.padding3, multiplexer, sizeof(multiplexer));
	
	sfd.unk3 = 0x08;
	sfd.unk4 = 0x02;
	sfd.unk5 = 0x08;
	
	sfd.total_streams = fd_size;
	
	// First stream entry is some TMP file
	const uint8_t tmp_name[] = "<SFM_P2>";
	const uint8_t tmp_ext[] = ".TMP";
	
	memcpy(sfd.stream_entries[0].audio.out_name, tmp_name, 8);
	memcpy(sfd.stream_entries[0].audio.out_ext, tmp_ext, 4);
	memcpy(sfd.stream_entries[0].video.out_date, sfd_date, 12);
	
	// Stream entries
	
	for(uint8_t i = 1; i != fd_size; ++i)
	{
		memset(&sfd.stream_entries[i].audio.out_name[0], 0x20, 8);
		memset(sfd.stream_entries[i].audio.out_ext, 0x20, 4);
		
		memcpy(sfd.stream_entries[i].audio.out_name,
			   fd[i].f_path.file_name.ptr,
			   fd[i].f_path.file_name.size >= 8 ? 8 : fd[i].f_path.file_name.size);
		
		sfd.stream_entries[i].audio.out_ext[0] = '.';
		memcpy(&sfd.stream_entries[i].audio.out_ext[1],
			   fd[i].f_path.ext.ptr,
			   fd[i].f_path.ext.size >= 3 ? 3 : fd[i].f_path.ext.size);
			   
		memcpy(sfd.stream_entries[i].video.out_date, sfd_date, 12);
		
		sfd.stream_entries[i].audio.stream_id = (fd[i].stream_id&0x000000FF);
		
		switch(fd[i].file_type)
		{
			case FD_ADX:
			case FD_AIX:
				sfd.audio_streams += 1;
				sfd.stream_entries[i].audio.channels = fd[i].data.adx.info.channel_count;
				sfd.stream_entries[i].audio.sample_rate = fd[i].data.adx.info.sample_rate;
				break;
			case FD_M1V:
				sfd.video_streams += 1;
				sfd.stream_entries[i].video.width = fd[i].data.m1v.width;
				sfd.stream_entries[i].video.height = change_order_16(fd[i].data.m1v.height);
				sfd.stream_entries[i].video.padding_FF[0] = 0xFF;
				sfd.stream_entries[i].video.padding_FF[1] = 0xFF;
				break;
		}
	}
	
	sfd.private_streams = 1;
	
	memset(&sfd.user_comment, 0x20, 8);
	memcpy(&sfd.user_comment, sofdec_comment, sizeof(sofdec_comment));
	
	fwrite(&sfd, sizeof(sfd_stream_s), 1, fd[0].f);
}

void sfd_sofdec2_mpeg_packet(File_desc* fd, const uint8_t fd_size)
{
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint16_t data_size = 60935; // 2030 LE
	fwrite(&mpeg_ps_priv2, sizeof(uint32_t), 1, fd[0].f);
	fwrite(&data_size, sizeof(uint16_t), 1, fd[0].f);
	
	sfd_stream2_s sfd;
	memset(&sfd, 0, sizeof(sfd_stream2_s));
	
	// sfd init
	memcpy(sfd.sofdec2_string, sofdec2_header, sizeof(sofdec2_header));
	
	sfd.unk1 = 0x08;
	sfd.unk2 = 0x02;
	sfd.unk3 = 0x01;
	sfd.unk4 = 0x02;
	sfd.unk5 = 0x27;
	
	memset(&sfd.muxer_program, ' ', sizeof(sfd.muxer_program));
	memcpy(sfd.muxer_program, multiplexer, sizeof(multiplexer));
	
	// Stream entries
	
	sfd.total_streams = fd_size;
	
	for(uint8_t i = 1; i != fd_size; ++i)
	{
		switch(fd[i].file_type)
		{
			case FD_ADX:
				sfd.audio_entries[sfd.audio_streams].audio_format = 1;
			case FD_AIX:
				if(sfd.audio_entries[sfd.audio_streams].audio_format == 0) 
					sfd.audio_entries[sfd.audio_streams].audio_format = 3;
				
				sfd.audio_entries[sfd.audio_streams].unk1 = 2;
				
				sfd.audio_entries[sfd.audio_streams].stream_id = fd[i].stream_id&0x000000FF;
				sfd.audio_entries[sfd.audio_streams].channels = fd[i].data.adx.info.channel_count;
				sfd.audio_entries[sfd.audio_streams].sample_rate = change_order_16(fd[i].data.adx.info.sample_rate);
				sfd.audio_entries[sfd.audio_streams].unk5 = fd[i].data.adx.info.sample_rate&0x00FF;
				
				sfd.audio_streams += 1;
				
				break;
			case FD_M1V:
				sfd.video_entries[sfd.video_streams].stream_id = fd[i].stream_id&0x000000FF;
				
				sfd.video_entries[sfd.video_streams].unk1 = 0x41;
				
				sfd.video_entries[sfd.video_streams].width = change_order_16(fd[i].data.m1v.width);
				sfd.video_entries[sfd.video_streams].height = change_order_16(fd[i].data.m1v.height);
				
				const uint32_t frames = mpeg1_framerate[fd[i].data.m1v.frame_rate]
										*fd[i].data.m1v.time_as_double;
				const uint16_t framerate = mpeg1_framerate[fd[i].data.m1v.frame_rate]*1000;
										
				sfd.video_entries[sfd.video_streams].frames = change_order_32(frames);
				sfd.video_entries[sfd.video_streams].framerate = change_order_16(framerate);
				
				sfd.video_entries[sfd.video_streams].unk7 = change_order_16(0x0100);
				sfd.video_entries[sfd.video_streams].unk8 = change_order_16(0x0010);
				sfd.video_entries[sfd.video_streams].unk9 = change_order_16(0x000F);
				sfd.video_entries[sfd.video_streams].unk10 = change_order_16(0x0300);
				
				sfd.video_streams += 1;
				
				break;
		}
	}
	
	sfd.private_streams = 1;
	
	fwrite(&sfd, sizeof(sfd_stream2_s), 1, fd[0].f);
}

void sfd_m1v_insert_userdata(sfd_frame_metadata_s* metadata, M1v_info* m1v)
{
	const uint32_t user_data_sync = change_order_32(M1V_USER_DATA);
	const uint64_t id_packet_size = (4+metadata->size);
	const uint64_t new_size = m1v->file_size + (id_packet_size*m1v->cur_frame);
	uint8_t* data_with_id = (uint8_t*)malloc(new_size);
	
	m1v->file_pos = 0;
	
	uint64_t it = 0;
	
	while(1)
	{
		const uint64_t packet_size = m1v_next_packet(m1v);
		const uint64_t packet_start = m1v->file_pos - packet_size;
		
		if(packet_size == 0) break;
		
		memcpy(&data_with_id[it], &m1v->file_buffer[packet_start], packet_size); 
		
		it += packet_size;
		
		if(m1v->last_stream_id == 0x00) // PIC_HEAD
		{
			memcpy(&data_with_id[it], &user_data_sync, 4);
			it += 4;
			memcpy(&data_with_id[it], metadata->data, metadata->size);
			it += metadata->size;
		}
	}
	
	free(m1v->file_buffer);
	
	m1v->file_buffer = data_with_id;
	m1v->file_size = new_size;
	m1v->file_pos = 0;
	m1v->cur_frame /= 2;
}

uint8_t sfd_csc_to_raw_y(const uint8_t value)
{
	return sfd_csc_y_lut[value];
}

uint8_t sfd_csc_to_raw_uv(const uint8_t value)
{
	return sfd_csc_uv_lut[value];
}