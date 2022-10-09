#include <m1v.h>

uint8_t m1v_init(const uint8_t* file_path, M1v_info* m1v)
{
	FILE* file_pointer;
	uint8_t ret = 0;
	
	memset(m1v, 0, sizeof(M1v_info));
	
	// Opening a file
	file_pointer = fopen(file_path, "rb");
	
	if(file_pointer == NULL)
	{
		ret = 1;
	}
	else
	{
		// Load file
		fseek(file_pointer, 0, SEEK_END);
		m1v->file_size = ftell(file_pointer);
		fseek(file_pointer, 0, SEEK_SET);
		m1v->file_buffer = (uint8_t*)malloc(m1v->file_size);
		
		if(m1v->file_buffer == NULL)
		{
			ret = 2;
		}
		else
		{
			fread(m1v->file_buffer, m1v->file_size, 1, file_pointer);
			fclose(file_pointer);
		}
	}
	
	return ret;
}

void m1v_destroy(M1v_info* m1v)
{
	if(m1v->file_buffer) free(m1v->file_buffer);
	if(m1v->user_data) free(m1v->user_data);
	if(m1v->frames_positions) free(m1v->frames_positions);
}

uint64_t m1v_next_packet(M1v_info* m1v)
{
	if(m1v->file_pos == m1v->file_size) return 0;
	
	uint8_t* data = &m1v->file_buffer[m1v->file_pos];
	const uint32_t sync = change_order_32(*(uint32_t*)&data[0]);
	m1v->last_stream_id = data[3];
	uint64_t ret = 4;
	
	switch(sync)
	{
		case M1V_SEQ_HEAD:
			ret += m1v_sequence_header(&data[4], m1v);
			break;
		case M1V_GOP:
			ret += m1v_gop(&data[4], m1v);
			break;
		case M1V_PIC_HEAD:
			ret += m1v_picture_header(&data[4], m1v);
			break;
		case M1V_USER_DATA:
			ret += m1v_user_data(&data[4], m1v);
			break;
		case M1V_EXTENSION:
			ret += m1v_extension(&data[4], m1v);
			break;
	}
	
	if(m1v_is_slice_sync(sync))
	{
		//printf("\tIt's a slice: %u\n", stream_id);
		ret += m1v_slice(&data[4], m1v, m1v->last_stream_id);
	}
	
	// If codec is not set, default to m1v.
	// Otherwise, it will be set to 2
	// when it enters M1V_EXTENSION.
	// I hope it's only used in MPEG-2 Part 2.
	if(m1v->codec == 0)
	{
		m1v->codec = 1;
	}
	
	m1v->file_pos += ret;
	
	return ret;
}

/*
	Checks
*/

uint8_t m1v_is_slice(const uint8_t stream_id)
{
	return ((stream_id >= M1V_SLICE_FIRST) && (stream_id <= M1V_SLICE_LAST));
}

uint8_t m1v_is_slice_sync(const uint32_t sync)
{
	const uint8_t stream_id = (sync&0x000000FF);
	const uint32_t prefix = (sync>>8);
	return ((prefix==1) && (stream_id >= M1V_SLICE_FIRST) && (stream_id <= M1V_SLICE_LAST));
}

uint64_t m1v_find_next_valid(const uint8_t* data, M1v_info* m1v)
{
	uint64_t ret = 0;
	uint8_t done = 0;
	
	// Check where tf the end is
	while(!done)
	{
		const uint32_t sync = change_order_32(*(uint32_t*)&data[ret]);
		
		switch(sync)
		{
			case M1V_SEQ_HEAD:
			case M1V_GOP:
			case M1V_PIC_HEAD:
			case M1V_EXTENSION:
			case M1V_USER_DATA:
				done = 1;
				break;
		}
		
		if(m1v_is_slice_sync(sync))
		{
			done = 1;
		}
		
		ret += 1;
		
		if((m1v->file_pos+ret+3) == m1v->file_size)
		{
			done = 1;
		}
	}
	
	ret -= 1;
	
	return ret;
}

M1v_info m1v_test_file(const uint8_t* filename)
{
	// Init m1v struct
	M1v_info m1v;
	const uint8_t m1v_status = m1v_init(filename, &m1v);
	
	if(m1v_status != 0)
	{
		m1v.codec = 0;
		return m1v;
	}

	// If 0, it'll stop parsing
	uint64_t finished = 1;
	
	while(finished)
	{
		finished = m1v_next_packet(&m1v);
		if(finished == 0) break;
		const uint8_t stream_id = m1v.last_stream_id;
		
		// Parse
		switch(stream_id)
		{
			case 0xB3: // Sequence Header
			case 0xB8: // Group of Pictures
			case 0x00: // Picture header
			case 0xB2: // User data
			case 0xB5: // Extension
				break;
			
			default:
				if(m1v_is_slice(stream_id) == 0) // Is slice
					finished = 0;
		}
	}

	//const uint8_t ret = (m1v.width*m1v.height*mpeg1_par[m1v.aspect_ratio]*mpeg1_framerate[m1v.frame_rate]) != 0 ? 255 : 0;
	
	// Cleanup
	//m1v_destroy(&m1v);
	
	return m1v;
}

/*
	m1v packets
*/

// M1V_SEQ_HEAD 0x000001B3
uint64_t m1v_sequence_header(const uint8_t* data, M1v_info* m1v)
{
	uint64_t ret = 0;
	const uint32_t first_4_bytes = change_order_32(*(uint32_t*)&data[0]);
	const uint32_t second_4_bytes = change_order_32(*(uint32_t*)&data[4]);
	ret+=8;
	
	m1v->width = (first_4_bytes&0xFFF00000)>>20;
	m1v->height = (first_4_bytes&0x000FFF00)>>8;
	m1v->aspect_ratio = (first_4_bytes&0x000000F0)>>4;
	m1v->frame_rate = (first_4_bytes&0x0000000F);
	m1v->bitrate = (second_4_bytes&0xFFFFC000)>>14;
	m1v->vbv_buf_size = (second_4_bytes&0x00001FF8)>>3;
	
	m1v->constrained_params_flag = (second_4_bytes&0x00000004)>>2;
	m1v->load_intra_quant_matrix = (second_4_bytes&0x00000002)>>1;
	//m1v->load_nonintra_quant_matrix = (second_4_bytes&0x00000001);
	
	if(m1v->load_intra_quant_matrix)
	{
		memcpy(m1v->intra_quant_matrix, &data[ret], 64);
		ret+=64;
	}
	
	m1v->load_nonintra_quant_matrix = (data[ret-1]&0x01);
	if(m1v->load_nonintra_quant_matrix)
	{
		memcpy(m1v->nonintra_quant_matrix, &data[ret], 64);
		ret+=64;
	}
	
	return ret;
}

// M1V_GOP 0x000001B8
uint64_t m1v_gop(const uint8_t* data, M1v_info* m1v)
{
	uint64_t ret = 0;
	const uint32_t first_4_bytes = change_order_32(*(uint32_t*)&data[0]);
	const uint16_t first_2_bytes = change_order_16(*(uint16_t*)&data[0]);
	ret+=4;
	
	m1v->drop_frame_flag = (first_4_bytes&0x80000000)>>31;
	m1v->time_hour = (first_4_bytes&0x7C000000)>>26;
	m1v->time_minute = (first_4_bytes&0x03F00000)>>20;
	m1v->time_second = (first_4_bytes&0x0007E000)>>13;
	m1v->gop_frame = (first_4_bytes&0x00001F80)>>7;
	m1v->closed_gop = (first_4_bytes&0x00000040)>>6;
	m1v->broken_gop = (first_4_bytes&0x00000020)>>5;
	
	m1v->time_milliseconds = ((m1v->gop_frame)/(mpeg1_framerate[m1v->frame_rate]))*1000;
	m1v->time_as_double = (m1v->time_hour*60*60)+(m1v->time_minute*60)+(m1v->time_second)+(m1v->time_milliseconds/1000.f);
	
	return ret;
}

// M1V_PIC_HEAD 0x00000100
uint64_t m1v_picture_header(const uint8_t* data, M1v_info* m1v)
{
	uint64_t ret = 0;
	const uint16_t first_2_bytes = change_order_16(*(uint16_t*)&data[0]);
	const uint32_t first_4_bytes = change_order_32(*(uint32_t*)&data[0]);
	const uint16_t last_2_bytes = change_order_16(*(uint16_t*)&data[3]); // If there are any appended fields
	ret+=4;
	
	m1v->temp_seq_num = (first_2_bytes&0xFFC0)>>6;
	m1v->frame_type = (data[1]&0x38)>>3;
	m1v->vbv_delay = (first_4_bytes&0x0007FFF8)>>3;
	
	if((m1v->frame_type == M1V_FRAME_P) || (m1v->frame_type == M1V_FRAME_B)) // Appended data
	{
		ret+=1;
		m1v->full_pel_forward_vector = (last_2_bytes&0x0400)>>10;
		m1v->forward_f_code = (last_2_bytes&0x0380)>>7;
		
		if(m1v->frame_type == M1V_FRAME_B) // Appended data for frame B
		{
			m1v->full_pel_backward_vector = (last_2_bytes&0x0040)>>6;
			m1v->backward_f_code = (last_2_bytes&0x0038)>>3;
		}
	}
	
	m1v->cur_frame += 1;
	m1v->frames_positions = (uint64_t*)realloc(m1v->frames_positions, sizeof(uint64_t)*m1v->cur_frame);
	m1v->frames_positions[m1v->cur_frame-1] = m1v->file_pos;
	
	return ret;
}

// M1V_SLICE 0x01-0xAF
uint64_t m1v_slice(const uint8_t* data, M1v_info* m1v, const uint8_t slice_id)
{
	const uint64_t end_pos = m1v_find_next_valid(data, m1v);
	m1v->slice_size = end_pos;
	
	return end_pos;
}

// M1V_USER_DATA 0x000001B2
uint64_t m1v_user_data(const uint8_t* data, M1v_info* m1v)
{
	if(m1v->user_data) free(m1v->user_data);
		
	const uint64_t end_pos = m1v_find_next_valid(data, m1v);
	
	m1v->user_data_size = end_pos+1;
	m1v->user_data = (uint8_t*)malloc(m1v->user_data_size);
	memset(m1v->user_data, 0, m1v->user_data_size);
	memcpy(m1v->user_data, data, m1v->user_data_size);
	
	return end_pos;
}

// M1V_EXTENSION 0x000001B5
uint64_t m1v_extension(const uint8_t* data, M1v_info* m1v)
{
	uint64_t ret = 0;
	
	const uint8_t ext_type = data[0]>>4;
	
	switch(ext_type)
	{
		// TODO: This. Will I need it? Probably not.
		// Would be great when decoding? Certainly.
		/*case 0x01: // Sequence_Extension
			break;
		case 0x02: // Sequence_Display_Extension
			break;
		case 0x07: // Picture_Display_Extension
			break;
		case 0x08: // Picture_Coding_Extension
			break;*/
			
		default:
			// Somehow, all cases failed.
			// Find next valid packet to process
			ret += m1v_find_next_valid(data, m1v);
	}
	
	// We now know it's MPEG-2 Part 2
	m1v->codec = 2;
	
	return ret;
}