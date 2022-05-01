#include "mpeg.h"
#include "utils.h"

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
			fread(&frame->frame_data_size, sizeof(uint16_t), 1, mpegfile);
			frame->len += 2;
			frame->frame_data_size = change_order_16(frame->frame_data_size);
			frame->len += frame->frame_data_size;
			
			frame->data = (uint8_t*)malloc(frame->len);
			fseek(mpegfile, -6, SEEK_CUR);
			fread(frame->data, 1, frame->len, mpegfile);
			
			frame->stream = STREAM_VIDEO;
			frame->av_id = i;
			
			// Checking if there is any 0xFF padding
			// and if extension is present
			uint16_t int_pos = 6;
			
			// If data[6] is equal to 0x0F
			// neither P-STD buffer and PTS/DTS are present
			if(frame->data[int_pos] == 0x0F)
			{
				break;
			}

			// Checking for padding
			while(frame->data[int_pos] == 0xFF)
			{
				int_pos += 1;
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
				break;
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
					}
					break;
			}
			
			break;
		}
	}

	// Checking if it's an audio stream
	// Don't need to check if previous check succeeded
	if(frame->stream == 0)
	{
		for(uint8_t i = 0; i < MPEG_MAX_AUDIO_STREAMS; ++i)
		{
			uint32_t bufSync = MPEG_AUDIO_START_ID;
			bufSync += i;
		
			if(frame->sync == bufSync)
			{			
				fread(&frame->frame_data_size, sizeof(uint16_t), 1, mpegfile);
				frame->len += 2;
				
				frame->frame_data_size = change_order_16(frame->frame_data_size);
				frame->len += frame->frame_data_size;
				
				frame->data = (uint8_t*)malloc(frame->len);
				fseek(mpegfile, -6, SEEK_CUR);
				fread(frame->data, 1, frame->len, mpegfile);
				
				frame->stream = STREAM_AUDIO;
				frame->av_id = i;
				
				/* Is it ADX? */
				if(strncmp(adx_frame_value, &frame->data[6], 2) == 0)
				{
					frame->is_adx = 1;
					frame->last_scr = mpeg_decode_scr(&frame->data[8]);
				}
				else
				{
					frame->last_scr = mpeg_decode_scr(&frame->data[6]);
				}
				
				break;
			}
		}
	}
	
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

void mpeg_write_prog_end(FILE* f)
{
	const uint32_t pe = change_order_32(PS_MPEG_PROGRAM_END);
	fwrite(&pe, sizeof(pe), 1, f);
	
	for(uint16_t i = 0; i != 2044; ++i)
	{
		fputc(0xFF, f);
	}
}

void mpeg_write_padding(FILE* file, const uint16_t padding_size)
{
	uint16_t size = padding_size;
	uint16_t size_be = 0;
	const uint32_t sync = change_order_32(PS_PADDING);
	
	if(size > 0x7FF)
	{
		size = 0x7FF;
	}
	
	size_be = change_order_16(size);
	
	fwrite(&sync, sizeof(uint32_t), 1, file);
	fwrite(&size_be, sizeof(uint16_t), 1, file);

	for(uint16_t i = 0; i != size; ++i)
	{
		fputc(0xFF, file);
	}
}