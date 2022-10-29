#include "sfd.h"

const static uint8_t sofdec_header[] = "SofdecStream            ";
const static uint8_t sofdec2_header[] = "SofdecStream2           ";
const uint8_t multiplexer[] = "CryTools sfdmux by Kwasior";
const static uint8_t sofdec_comment[0x40] = "Created using sfdmux https://github.com/ThisKwasior/CryTools    ";

void sfd_sofdec_mpeg_packet(File_desc* fd, const uint8_t fd_size)
{
	// MPEG-PS packet header and payload size.
	// No extension present so we can just use the size of sfd_stream_s.
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint16_t data_size = change_order_16(sizeof(sfd_stream_s));
	fwrite(&mpeg_ps_priv2, sizeof(uint32_t), 1, fd[0].f);
	fwrite(&data_size, sizeof(uint16_t), 1, fd[0].f);
	
	// Filling sfd_stream with data
	sfd_stream_s sfd;
	memset(&sfd, 0, sizeof(sfd_stream_s));
	
	memcpy(sfd.sofdec_string, sofdec_header, sizeof(sofdec_header));
	sfd.unk_0x26 = 0x02;
	sfd.unk_0x27 = 0x1C;
	
	memset(sfd.out_file.sofdec.out_name, 0x20, 8);
	memcpy(sfd.out_file.sofdec.out_name,
		   fd[0].f_path.file_name.ptr,
		   fd[0].f_path.file_name.size >= 8 ? 8 : fd[0].f_path.file_name.size);
	const uint8_t sfd_ext[] = ".sfd";
	memcpy(sfd.out_file.sofdec.out_ext, sfd_ext, 4);
	get_current_date_as_str(sfd.out_file.sofdec.out_date);
	memcpy(sfd.out_file.sofdec.muxer_name, multiplexer, sizeof(multiplexer));
	
	sfd.mpeg_pack_size = sfd.mpeg_pack_size_again = MPEG_DVD_BLOCK; // 2048
	sfd.unk_0x76 = 0x02000000;
	
	memcpy(sfd.user_comment_0xCE, sofdec_comment, sizeof(sofdec_comment));
	
	// First stream entry is some TMP file
	const uint8_t tmp_name[] = "<SFM_P2>";
	const uint8_t tmp_ext[] = ".TMP";
	memcpy(sfd.reserved_stream.sofdec.out_name, tmp_name, 8);
	memcpy(sfd.reserved_stream.sofdec.out_ext, tmp_ext, 4);
	get_current_date_as_str(sfd.reserved_stream.sofdec.out_date);
	sfd.reserved_stream.sofdec.padding[0] = 0xBF;
	
	// Video/Audio entries
	sfd.audio_streams = 0;
	sfd.video_streams = 0;
	sfd.private_streams = 1;
	
	for(uint8_t i = 1; i != fd_size; ++i)
	{
		// Not to have "[i-1]" hell
		sfd_metadata_file_s* cur_entry = &sfd.stream_entries[i-1];
		File_desc* fdp = &fd[i];
		File_path_s* fpp = &fd[i].f_path;
		
		memset(cur_entry->audio.out_name, 0x20, 8);
		memset(cur_entry->audio.out_ext, 0x20, 8);
		get_current_date_as_str(cur_entry->audio.out_date);
		
		memcpy(cur_entry->audio.out_name,
			   fpp->file_name.ptr,
			   fpp->file_name.size >= 8 ? 8 : fpp->file_name.size);
			   
		cur_entry->audio.out_ext[0] = '.';
		memcpy(&cur_entry->audio.out_ext[1],
			   fpp->ext.ptr,
			   fpp->ext.size >= 3 ? 3 : fpp->ext.size);
			   
		cur_entry->audio.stream_id = fdp->stream_id&0x000000FF;
		
		switch(fdp->file_type)
		{
			case FD_ADX:
				sfd.audio_streams += 1;
				cur_entry->audio.audio_type = SFD_STREAM_AUDIO_TYPE_ADX;
				cur_entry->audio.channels = fdp->data.adx.info.channel_count;
				cur_entry->audio.sample_rate = fdp->data.adx.info.sample_rate;
				
				if(sfd.longest_audio_len < (fdp->data.adx.est_length*10000))
					sfd.longest_audio_len = (fdp->data.adx.est_length*10000);
				
				break;
			case FD_AIX:
				sfd.audio_streams += 1;
				cur_entry->audio.audio_type = SFD_STREAM_AUDIO_TYPE_AIX;
				cur_entry->audio.channels = fdp->data.adx.info.channel_count;
				cur_entry->audio.sample_rate = fdp->data.adx.info.sample_rate;
				
				if(sfd.longest_audio_len < (fdp->data.adx.est_length*10000))
					sfd.longest_audio_len = (fdp->data.adx.est_length*10000);
				
				break;
			case FD_M1V:
				sfd.video_streams += 1;
				cur_entry->video.stream_id_01 = 0x01;
				cur_entry->video.padding_0x1A = 0xFFFF;
				cur_entry->video.width = (fdp->data.m1v.width&0xFF00)>>8;
				cur_entry->video.height = change_order_16(fdp->data.m1v.height);
				
				if(sfd.max_num_video_frames < fdp->data.m1v.cur_frame)
					sfd.max_num_video_frames = fdp->data.m1v.cur_frame;
				
				if(sfd.longest_video_len < fdp->data.m1v.time_milliseconds)
					sfd.longest_video_len = fdp->data.m1v.time_milliseconds;
				
				break;
		}
	}
	
	sfd.total_streams = sfd.audio_streams + sfd.video_streams + sfd.private_streams;
	
	fwrite(&sfd, sizeof(sfd_stream_s), 1, fd[0].f);
}

void sfd_sofdec2_mpeg_packet(File_desc* fd, const uint8_t fd_size)
{
	// MPEG-PS packet header and payload size.
	// No extension present so we can just use the size of sfd_stream_s.
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint16_t data_size = change_order_16(sizeof(sfd_stream2_s));
	fwrite(&mpeg_ps_priv2, sizeof(uint32_t), 1, fd[0].f);
	fwrite(&data_size, sizeof(uint16_t), 1, fd[0].f);
	
	// Filling sfd_stream with data
	sfd_stream2_s sfd;
	memset(&sfd, 0, sizeof(sfd_stream2_s));
	
	sfd.mpeg_pack_size = change_order_16(MPEG_DVD_BLOCK);
	
	memcpy(sfd.sofdec_string, sofdec2_header, sizeof(sofdec2_header));
	
	sfd.unk_0x26 = 0x02;
	sfd.unk_0x27 = 0x03;
	sfd.unk_0x28 = 0x02;
	sfd.unk_0x29 = 0x49;
	
	memset(sfd.muxer_name, 0x20, sizeof(sfd.muxer_name));
	memcpy(sfd.muxer_name, multiplexer, sizeof(multiplexer));
	
	// Video/Audio entries
	sfd.audio_streams = 0;
	sfd.video_streams = 0;
	sfd.private_streams = 1;
	
	for(uint8_t i = 1; i != fd_size; ++i)
	{
		sfd_stream2_audio_metadata_s* audio_entry = &sfd.audio_entries[sfd.audio_streams];
		sfd_stream2_video_metadata_s* video_entry = &sfd.video_entries[sfd.video_streams];
		File_desc* fdp = &fd[i];
		const uint8_t stream_id = fdp->stream_id&0x000000FF;
		uint8_t sample_byte = 0;
		uint8_t sample_byte2 = 0;
		
		switch(fdp->file_type)
		{
			case FD_ADX:
				sfd.audio_streams += 1;
				audio_entry->stream_id = stream_id;
				audio_entry->unk_0x01_0 = 2;
				audio_entry->audio_format = SFD_STREAM2_AUDIO_TYPE_ADX;
				audio_entry->channels = fdp->data.adx.info.channel_count;
				audio_entry->sample_rate = change_order_16(fdp->data.adx.info.sample_rate);
				
				sample_byte = (audio_entry->sample_rate&0x00FF);
				sample_byte2 = (audio_entry->sample_rate&0xFF00)>>8;
				audio_entry->rate_x_duration = change_order_32(sample_byte*fdp->data.adx.est_length);
				audio_entry->unk_0x0D = sample_byte2;
				
				break;
			case FD_AIX:
				sfd.audio_streams += 1;
				audio_entry->stream_id = stream_id;
				audio_entry->unk_0x01_0 = 2;
				audio_entry->audio_format = SFD_STREAM2_AUDIO_TYPE_AIX;
				audio_entry->channels = fdp->data.adx.info.channel_count;
				audio_entry->sample_rate = change_order_16(fdp->data.adx.info.sample_rate);
				
				sample_byte = (audio_entry->sample_rate&0x00FF);
				sample_byte2 = (audio_entry->sample_rate&0xFF00)>>8;
				audio_entry->rate_x_duration = change_order_32(sample_byte*fdp->data.adx.est_length);
				audio_entry->unk_0x0D = sample_byte2;
				
				break;
			case FD_M1V:
				sfd.video_streams += 1;
				video_entry->stream_id = stream_id;
				video_entry->unk_0x01 = 0x41;
				video_entry->width = change_order_16(fdp->data.m1v.width);
				video_entry->height = change_order_16(fdp->data.m1v.height);
				
				const uint32_t frames = mpeg1_framerate[fdp->data.m1v.frame_rate]
										*fdp->data.m1v.time_as_double;
				const uint16_t framerate = mpeg1_framerate[fdp->data.m1v.frame_rate]*1000;
				
				video_entry->frames = change_order_32(frames);
				video_entry->framerate = change_order_16(framerate);
				
				video_entry->unk_0x20 = change_order_16(0x0100);
				video_entry->unk_0x22 = change_order_16(0x0010);
				video_entry->unk_0x24 = change_order_16(0x000F);
				video_entry->unk_0x26 = change_order_16(0x0300);
				video_entry->unk_0x28 = change_order_16(fdp->data.m1v.width);
				video_entry->unk_0x2A = change_order_16(fdp->data.m1v.height);
				
				break;
		}
	}
	
	sfd.total_streams = sfd.audio_streams + sfd.video_streams + sfd.private_streams;
	
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