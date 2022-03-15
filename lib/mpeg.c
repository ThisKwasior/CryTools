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
			
			frame->last_scr = mpeg1_decode_scr(&frame->data[4]);
			
			frame->stream = -1;
			break;
		case PS_PARTIAL_HEADER:
		case PS_PADDING:
		case PS_PRIV2:
			fread(&frame->frame_data_size, sizeof(uint16_t), 1, mpegfile);
			frame->len += 2;
			frame->frame_data_size = change_order_16(frame->frame_data_size);
			frame->len += frame->frame_data_size;
			
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
			
			/* Something weird with data in some of my encodings */
			if((frame->data[6] != 0xFF) && (frame->data[6] != 0xF))
			{
				//printf("Weird: %x\n", frame->data[6]);
				frame->last_scr = mpeg1_decode_scr(&frame->data[6]);
				//printf("Weird SCR: %d\n", frame->last_scr);
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
					frame->last_scr = mpeg1_decode_scr(&frame->data[8]);
				}
				else
				{
					frame->last_scr = mpeg1_decode_scr(&frame->data[6]);
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

uint64_t mpeg1_decode_scr(const uint8_t* const scr_array)
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

void mpeg1_encode_scr(uint8_t* arr, const uint64_t scr_value)
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

void mpeg1_write_prog_end(FILE* f)
{
	//const uint8_t pe[] = {0, 0, 1, 0xB9};
	const uint32_t pe = change_order_32(PS_MPEG_PROGRAM_END);
	fwrite(&pe, sizeof(pe), 1, f);
	
	for(uint16_t i = 0; i != 2044; ++i)
	{
		fputc(0xFF, f);
	}
}