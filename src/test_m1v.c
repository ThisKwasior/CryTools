#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <common.h>
#include <io_common.h>
#include <m1v.h>

int main(int argc, char** argv)
{
	FILE* flog = fopen("m1vlog.txt", "wb");
	
	printf("====Name: %s\n", argv[1]);
	fprintf(flog, "====Name: %s\n", argv[1]);
	
	// Init m1v struct
	M1v_info m1v;
	const uint8_t m1v_status = m1v_init(argv[1], &m1v);
	
	if(m1v_status != 0)
	{
		printf("====Something wrong with the file. Status: %u\n", m1v_status);
		fprintf(flog, "====Something wrong with the file. Status: %u\n", m1v_status);
	}
	
	printf("====Size: %u\n", m1v.file_size);
	fprintf(flog, "====Size: %u\n", m1v.file_size);
	
	// Check stuff
	uint64_t finished = 1;
	
	while(finished)
	{
		finished = m1v_next_packet(&m1v);
		if(finished == 0) break;
		const uint8_t stream_id = m1v.last_stream_id;
		fprintf(flog, "==Stream ID: %02x | %x\n", stream_id, m1v.file_pos);
		
		// Parse
		switch(stream_id)
		{
			case 0xB3: // Sequence Header
				fprintf(flog, "\tSize: %hu %hu\n", m1v.width, m1v.height);
				fprintf(flog, "\tAspect ratio: %.3lf\n", mpeg1_par[m1v.aspect_ratio]);
				fprintf(flog, "\tFramerate: %.3lf\n", mpeg1_framerate[m1v.frame_rate]);
				fprintf(flog, "\tBitrate: %u\n", m1v.bitrate);
				fprintf(flog, "\tVBV: %hu\n", m1v.vbv_buf_size);
				fprintf(flog, "\tFlags: %u%u%u\n", m1v.constrained_params_flag, m1v.load_intra_quant_matrix, m1v.load_nonintra_quant_matrix);
				break;
				
			case 0xB8: // Group of Pictures
				fprintf(flog, "\tdrop_frame_flag: %u\n", m1v.drop_frame_flag);
				fprintf(flog, "\ttime_hour: %u\n", m1v.time_hour);
				fprintf(flog, "\ttime_minute: %u\n", m1v.time_minute);
				fprintf(flog, "\ttime_second: %u\n", m1v.time_second);
				fprintf(flog, "\tgop_frame: %u\n", m1v.gop_frame);
				fprintf(flog, "\tclosed_gop: %u\n", m1v.closed_gop);
				fprintf(flog, "\tbroken_gop: %u\n", m1v.broken_gop);
				break;
				
			case 0x00: // Picture header
				fprintf(flog, "\ttemp_seq_num: %u\n", m1v.temp_seq_num);	// temporal sequence number
				fprintf(flog, "\tframe_type: %u\n", m1v.frame_type);		// 1=I, 2=P, 3=B, 4=D
				fprintf(flog, "\tvbv_delay: %u\n", m1v.vbv_delay);
				
				if((m1v.frame_type == M1V_FRAME_P) || (m1v.frame_type == M1V_FRAME_B))
				{
					fprintf(flog, "\tfull_pel_forward_vector: %u\n", m1v.full_pel_forward_vector);
					fprintf(flog, "\tforward_f_code: %u\n", m1v.forward_f_code);
					
					if(m1v.frame_type == M1V_FRAME_B)
					{
						fprintf(flog, "\tfull_pel_backward_vector: %u\n", m1v.full_pel_backward_vector);
						fprintf(flog, "\tbackward_f_code: %u\n", m1v.backward_f_code);
					}
				}
				break;
				
			case 0xB2: // User data
				fprintf(flog, "\tuser_data: ");
				for(uint32_t i = 0; i != m1v.user_data_size-1; ++i)
				{
					fprintf(flog, "%c", m1v.user_data[i]);
				}
				fprintf(flog, "\n");
				fprintf(flog, "\tuser_data_size: %u\n",  m1v.user_data_size);
				break;

			case 0xB5: // Extension
				fprintf(flog, "\tExtension size: %u\n", finished-4);
				break;			
			
			default:
				if(m1v_is_slice(stream_id)) // Is slice
				{
					fprintf(flog, "\tSlice size: %x\n", m1v.slice_size);
				}
				else
				{
					finished = 0;
				}
		}
	}
	
	// Print in console
	printf("\tResolution: %hux%hu\n", m1v.width, m1v.height);
	printf("\tCodec: %u\n", m1v.codec);
	
	if(m1v.codec == 1) printf("\tAspect ratio: %.3lf\n", mpeg1_par[m1v.aspect_ratio]);
	else if(m1v.codec == 2) printf("\tAspect ratio: %.3lf\n", mpeg2_dar[m1v.aspect_ratio]);

	printf("\tFramerate: %.3lf\n", mpeg1_framerate[m1v.frame_rate]);
	const uint32_t milliseconds = ((m1v.gop_frame)/(mpeg1_framerate[m1v.frame_rate]))*1000;
	printf("\tLength: %02u:%02u:%02u.%03u\n", m1v.time_hour, m1v.time_minute, m1v.time_second, milliseconds);
	
	// Cleanup
	m1v_destroy(&m1v);
	
	fclose(flog);
}