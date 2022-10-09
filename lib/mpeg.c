#include "mpeg.h"

void mpeg_get_next_frame(FILE* mpegfile, struct Mpeg1Frame* frame)
{	
	frame->sync = 0;
	frame->len = 0;
	frame->frame_data_size = 0;
	frame->stream = 0;
	frame->av_id = -1;
	frame->is_adx = 0;
	
	fread(&frame->sync, sizeof(uint32_t), 1, mpegfile);
	frame->len = 4;

	frame->sync = change_order_32(frame->sync);

	free(frame->data);

	switch(frame->sync)
	{
		case PS_PACK_HEADER:
			frame->len += 8;
			frame->data = (uint8_t*)malloc(frame->len);
			fseek(mpegfile, -4, SEEK_CUR);
			fread(frame->data, 1, frame->len, mpegfile);
			
			frame->last_scr = mpeg_decode_scr(&frame->data[4]);
			
			frame->stream = -1;
			break;
		case PS_SYSTEM_HEADER:
		case PS_PADDING:
		case PS_PRIV2:
			fread(&frame->frame_data_size, sizeof(uint16_t), 1, mpegfile);
			frame->len += 2;
			frame->frame_data_size = change_order_16(frame->frame_data_size);
			frame->len += frame->frame_data_size;
			
			// 'data' holds entire packet, with sync and length
			frame->data = (uint8_t*)malloc(frame->len);
			fseek(mpegfile, -6, SEEK_CUR);
			fread(frame->data, 1, frame->len, mpegfile);
			break;
		case PS_MPEG_PROGRAM_END:
			frame->data = (uint8_t*)malloc(MPEG_MAX_DATA_SIZE);
			frame->len += fread(frame->data, 1, MPEG_MAX_DATA_SIZE, mpegfile);
			break;
	}
	
	// Checking if it's a video stream
	for(uint8_t i = 0; i < MPEG_MAX_VIDEO_STREAMS; ++i)
	{
		uint32_t bufSync = MPEG_VIDEO_START_ID;
		bufSync += i;
	
		if(frame->sync == bufSync)
		{	
			frame->stream = STREAM_VIDEO;
			frame->av_id = i;
			goto prep_multimedia_packet;
		}
	}
	
	// or audio stream
	for(uint8_t i = 0; i < MPEG_MAX_AUDIO_STREAMS; ++i)
	{
		uint32_t bufSync = MPEG_AUDIO_START_ID;
		bufSync += i;
		
		if(frame->sync == bufSync)
		{
			frame->stream = STREAM_AUDIO;
			frame->av_id = i;
			goto prep_multimedia_packet;
		}
	}
	
	goto nothing_found;
	
prep_multimedia_packet:

	frame->len = 0; // hack before refactor
					// now data will hold m1v data only
	fread(&frame->frame_data_size, sizeof(uint16_t), 1, mpegfile);
	frame->frame_data_size = change_order_16(frame->frame_data_size);
	frame->len += frame->frame_data_size;
	
	frame->data = (uint8_t*)malloc(frame->len);
	fread(frame->data, 1, frame->len, mpegfile);
	
	// Checking if there is any 0xFF padding
	// and if extension is present
	uint16_t int_pos = 0;

	// Checking for padding
	while(frame->data[int_pos] == 0xFF)
	{
		int_pos += 1;
	}
	
	// If data[6] is equal to 0x0F
	// neither P-STD buffer and PTS/DTS are present
	
	if(frame->data[int_pos] == 0x0F)
	{
		int_pos += 1;
		goto finish_packet_prep;
	}

	// Extension
	uint8_t val_bits = frame->data[int_pos]>>6;
	
	// STD buffer size field
	frame->p_std_buf_scale = -1;
	frame->p_std_buf_size = -1;
	
	if(val_bits == 1)
	{
		// Extracting bit 5
		// Buffer scale
		frame->p_std_buf_scale = (frame->data[int_pos]&0x20)>>5;
		
		// Extracting buffer size
		// 13 bytes
		const uint16_t field = *(uint16_t*)(&frame->data[int_pos]);
		frame->p_std_buf_size = field&0x1FFF;
		
		int_pos += 2;
	}
	
	// Checking for PTS/DTS
	
	// Once again, comparing current byte to 0x0F
	if(frame->data[int_pos] == 0x0F)
	{
		int_pos += 1;
		goto finish_packet_prep;
	}
	
	val_bits = frame->data[int_pos]>>4;
	
	switch(val_bits)
	{
		// PTS only, '0010'
		case 2:
			frame->last_pts = mpeg_decode_scr(&frame->data[int_pos]);
			int_pos += 5;
			break;
			
		// PTS and DTS, '0011'
		// After PTS there's another check for DTS, which is '0001'
		case 3:
			frame->last_pts = mpeg_decode_scr(&frame->data[int_pos]);
			int_pos += 5;
			
			val_bits = frame->data[int_pos]>>4;
			if(val_bits == 1)
			{
				frame->last_dts = mpeg_decode_scr(&frame->data[int_pos]);
				int_pos += 5;
			}
			break;
	}

finish_packet_prep:

	// Only m1v data
	frame->len -= int_pos;
	memcpy(&frame->data[0], &frame->data[int_pos], frame->len);

nothing_found:

	if(!frame->data && frame->sync != 0)
	{
		printf("Unknown sync %x.\n", frame->sync);
	}
}

uint64_t mpeg_decode_scr(const uint8_t* const scr_array)
{
    uint64_t decoded = 0;
    
    uint64_t first      = (scr_array[0]&14)>>1;
    uint64_t second     = scr_array[1];
    uint64_t third      = (scr_array[2]&254)>>1;
    uint64_t fourth     = scr_array[3];
    uint64_t fifth      = (scr_array[4]&254)>>1;

    decoded |= (first<<30);
    decoded |= (second<<22);
    decoded |= (third<<15);
    decoded |= (fourth<<7);
    decoded |= fifth;
    
    return decoded;
}

void mpeg_encode_scr(uint8_t* arr, const uint64_t scr_value)
{
	uint64_t buf = 0;

	arr[0] |= 33;
	buf = scr_value>>29;
	arr[0] |= buf;
	
	buf = (scr_value>>22)&0xFF;
	arr[1] |= buf;
	
	arr[2] |= 1;
	buf = (scr_value>>14)&254;
	arr[2] |= buf;
	
	buf = (scr_value>>7)&0xFF;
	arr[3] |= buf;
	
	arr[4] |= 1;
	buf = (scr_value<<1)&254;
	arr[4] |= buf;
}

uint8_t mpeg_encode_pts_dts(uint8_t* arr, const uint64_t pts, const uint64_t dts)
{
	if(pts != -1)
	{
		mpeg_encode_scr(&arr[0], pts);
		
		if(dts != -1)
		{
			mpeg_encode_scr(&arr[5], dts);
			arr[0] |= (1<<4); // 4bit header changes to 0011b
			
			arr[5] ^= (1<<4); // dts header is 0001b
			arr[5] ^= (1<<5); // dts header is 0001b
			
			return 10;
		}
		return 5;
	}
	else
	{
		arr[0] = 0x0F;
		return 1;
	}
}

void mpeg_write_prog_end(FILE* file)
{
	uint8_t buffer[MPEG_DVD_BLOCK] = {0};
	
	const uint16_t ret = mpeg_prep_stream_packet(&buffer[0], PS_MPEG_PROGRAM_END,
												 NULL, 0, MPEG_DVD_BLOCK,
												 0, 0, 
												 0, 0);
												 
	fwrite(&buffer[0], ret, 1, file);
}

void mpeg_write_padding(FILE* file, const uint16_t padding_size)
{
	uint8_t buffer[MPEG_DVD_BLOCK] = {0};
	
	const uint16_t ret = mpeg_prep_stream_packet(&buffer[0], PS_PADDING,
												 NULL, 0, padding_size,
												 0, 0, 
												 0, 0);
												 
	fwrite(&buffer[0], ret, 1, file);
}

void mpeg_write_pack_header(FILE* file, const uint64_t scr, const uint32_t mux_rate)
{
	const uint32_t sync = change_order_32(PS_PACK_HEADER);
	uint8_t scr_out[5] = {0};
	uint8_t mux_out[3] = {0x8E, 0xA6, 0x01};
	
	mpeg_encode_scr(scr_out, scr);
	
	fwrite(&sync, sizeof(uint32_t), 1, file);
	fwrite(scr_out, sizeof(uint8_t), 5, file);
	fwrite(mux_out, sizeof(uint8_t), 3, file);
}

uint16_t mpeg_write_system_header(FILE* file, const uint8_t audio_bound, const uint8_t video_bound)
{
	const uint32_t sync = change_order_32(PS_SYSTEM_HEADER);
	const uint16_t payload_size = 6 + (audio_bound*3) + (video_bound*3);
	const uint16_t payload_size_be = change_order_16(payload_size);
	const uint16_t ret = 4 + 2 + payload_size;
	
	uint8_t* buffer = (uint8_t*)malloc(ret);
	
	//const uint8_t rate_bound[3] = {0x88, 0x24, 0x29};
	//const uint8_t rate_bound[3] = {0x89, 0xC4, 0x01}; // 320000
	const uint8_t rate_bound[3] = {0x8E, 0xA6, 0x01}; // 480000
	uint8_t after_rate[3] = {0, 0, 0};
	
	after_rate[0] += (audio_bound<<2); // audio_bound
	after_rate[0] |= (1<<1); // fixed_flag
	after_rate[1] |= (1<<5); // separator
	after_rate[1] += video_bound; // video_bound
	after_rate[2] = 0xFF;

	uint16_t it = 0;
	memcpy(&buffer[it], &sync, sizeof(uint32_t));
	it += sizeof(uint32_t);
	memcpy(&buffer[it], &payload_size_be, sizeof(uint16_t));
	it += sizeof(uint16_t);
	memcpy(&buffer[it], &rate_bound, sizeof(uint8_t)*3);
	it += sizeof(uint8_t)*3;
	memcpy(&buffer[it], &after_rate, sizeof(uint8_t)*3);
	it += sizeof(uint8_t)*3;
	
	for(uint8_t i = 0; i != audio_bound; ++i)
	{
		const uint8_t audio_start = MPEG_AUDIO_START_ID&0x000000FF;
		const uint8_t audio_current = audio_start + i;
		const uint8_t STD_buffer_size_bound = 0x04;
		
		buffer[it++] = audio_current;
		buffer[it++] = audio_start;
		buffer[it++] = STD_buffer_size_bound;
	}
	
	for(uint8_t i = 0; i != video_bound; ++i)
	{
		const uint8_t video_start = MPEG_VIDEO_START_ID&0x000000FF;
		const uint8_t video_current = video_start + i;
		const uint8_t STD_buffer_size_bound = 0x2E;
		
		buffer[it++] = video_current;
		buffer[it++] = video_start;
		buffer[it++] = STD_buffer_size_bound;
	}
	
	fwrite(buffer, ret, 1, file);
	
	free(buffer);
	return ret;
}

uint16_t mpeg_prep_stream_packet(uint8_t* buffer, const uint32_t stream_id,
								 const uint8_t* data, const uint64_t data_size, uint16_t available,
								 const uint8_t buffer_scale, const uint16_t buffer_size,
								 const uint64_t pts, const uint64_t dts)
{
	memset(buffer, 0, MPEG_DVD_BLOCK);
	
	const uint32_t sync = change_order_32(stream_id);
	uint8_t encode_ret = 0;
	uint16_t data_size_be = 0;
	uint16_t ret = 4;
	
	switch(stream_id)
	{
		case PS_MPEG_PROGRAM_END:
			memset(buffer, 0xFF, MPEG_DVD_BLOCK);
			memcpy(&buffer[0], &sync, 4);
			ret = MPEG_DVD_BLOCK;
			break;
			
		case PS_PADDING:
			memcpy(&buffer[0], &sync, 4);
			
			ret = available;
			available -= 6;
			
			data_size_be = change_order_16(available);
			memcpy(&buffer[4], &data_size_be, 2);
			
			memset(&buffer[6], 0xFF, available);
			buffer[6] = 0x0F;
			break;

		default:
			// Audio/Video stream?
			memcpy(&buffer[0], &sync, 4);
			
			available -= 4; // sync
			available -= 2; // size
			
			if(buffer_size)
			{
				buffer[6] |= (1<<6); // Bit field for STD buffer
				if(buffer_scale) buffer[6] |= (1<<5); // Bit field for STD buffer
				buffer[6] += (buffer_size>>8)&0x1F;
				buffer[7] += buffer_size&0xFF;
				data_size_be += 2;
				available -= 2;
				
				encode_ret = mpeg_encode_pts_dts(&buffer[8], pts, dts);

				data_size_be += encode_ret;
				available -= encode_ret;
			}
			else
			{
				buffer[6] = 0x0F;
				data_size_be += 1;
				available -= 1;
			}

			if(available > data_size) available = data_size;

			ret = available;

			memcpy(&buffer[6+data_size_be], &data[0], available); // Stream payload

			data_size_be = change_order_16(available + data_size_be); // Payload size in BE
			memcpy(&buffer[4], &data_size_be, 2);
	}
	
	return ret;
}