#include "adx.h"

ADX read_adx_info(FILE* adx_file)
{
	ADX adx;
	
	/* 
		Checking if the file is indeed ADX or AIX.
		If not, magic will be 0xFFFF
	*/
	
	const uint8_t is_valid = check_adx_file(adx_file);
	rewind(adx_file);
	
	switch(is_valid)
	{
		// Not a valid file
		case 0:
			adx.info.magic = 0xFFFF;
			break;
		
		// ADX
		case 1:
			fread(&adx.info, sizeof(struct AdxBasicInfo), 1, adx_file);
			
			if(adx.info.version == 3)
			{
				fread(&adx.unknown_loop, 4, 1, adx_file);
			}
			else if(adx.info.version == 4)
			{
				fread(&adx.unknown_loop, 16, 1, adx_file);
			}

			fread(&adx.loop, sizeof(struct AdxLoop), 1, adx_file);

			/* Values are BE, so we need to convert these for x86 */
			adx.info.magic = change_order_16(adx.info.magic);
			adx.info.copyright_offset = change_order_16(adx.info.copyright_offset);
			adx.info.sample_rate = change_order_32(adx.info.sample_rate);
			adx.info.sample_count = change_order_32(adx.info.sample_count);
			adx.info.highpass_freq = change_order_16(adx.info.highpass_freq);
			
			adx.loop.loop_flag = change_order_32(adx.loop.loop_flag);
			adx.loop.loop_start_sample = change_order_32(adx.loop.loop_start_sample);
			adx.loop.loop_start_byte = change_order_32(adx.loop.loop_start_byte);
			adx.loop.loop_end_sample = change_order_32(adx.loop.loop_end_sample);
			adx.loop.loop_end_byte = change_order_32(adx.loop.loop_end_byte);

			/* Get the estimated length in seconds */
			adx.est_length = adx.info.sample_count/(float)adx.info.sample_rate;
			break;
		
		// AIX
		// TODO: Separate it from ADX stuff
		case 2:
			// "AI" string, part of "AIXF" header
			adx.info.magic = 0x4149;
			
			uint32_t adx_size = 0;
			uint8_t adx_files = 0;
			uint16_t buff_16 = 0;
			uint16_t buff_32 = 0;
			
			fseek(adx_file, 4, SEEK_CUR); 									// Magic
			fseek(adx_file, 2, SEEK_CUR); 									// Copyright offset
			fread(&buff_16, sizeof(uint16_t), 1, adx_file);
			adx.info.copyright_offset = change_order_16(buff_16);
			fseek(adx_file, 2, SEEK_CUR);									// always 1?
			fread(&buff_16, sizeof(uint16_t), 1, adx_file);					// Block size
			adx.info.block_size = (uint8_t)change_order_16(buff_16);
			fseek(adx_file, 4, SEEK_CUR);									// No idea
			fseek(adx_file, 0x10, SEEK_CUR);								// No idea #2
			fseek(adx_file, 4, SEEK_CUR);									// No idea #3
			fread(&adx_size, sizeof(uint32_t), 1, adx_file);				// ADX size in AIX
			fread(&adx.info.sample_count, sizeof(uint32_t), 1, adx_file);	// Sample count
			adx.info.sample_count = change_order_32(adx.info.sample_count);
			fread(&adx.info.sample_rate, sizeof(uint32_t), 1, adx_file);	// Sample rate
			adx.info.sample_rate = change_order_32(adx.info.sample_rate);
			fseek(adx_file, 0x10, SEEK_CUR);								// No idea #4
			fread(&adx_files, sizeof(uint8_t), 1, adx_file);				// ADX files in AIX
			fseek(adx_file, 0x9, SEEK_CUR);									// No idea #5
			fread(&buff_16, sizeof(uint16_t), 1, adx_file);					// Sample rate of first file
			fread(&adx.info.channel_count, sizeof(uint8_t), 1, adx_file);	// Channel count
			
			/* Get the estimated length in seconds */
			adx.est_length = adx.info.sample_count/(float)adx.info.sample_rate;
			
			break;
			
		default:
			adx.info.magic = 0xFFFF;
	}

	fseek(adx_file, 0, SEEK_END);
	adx.file_size = ftell(adx_file);
	rewind(adx_file);

	return adx;
}

uint8_t check_adx_file(FILE* adx_file)
{
	const uint8_t magic_adx[2] = {0x80, 0x00};
	uint8_t magic[6];
	fread(magic, sizeof(uint8_t), 4, adx_file);
	
	if(strncmp(magic, "AIXF", 4) == 0)
	{
		return 2;
	}
	
	if(strncmp(magic, magic_adx, 2) == 0)
	{
		uint8_t is_adx = 0;
		const uint16_t cp_offset = change_order_16(*(uint16_t*)&magic[2]);
		
		fseek(adx_file, cp_offset-2, SEEK_SET);
		fread(magic, sizeof(uint8_t), 6, adx_file);
		
		if(strncmp(magic, "(c)CRI", 6) == 0)
		{
			is_adx += 1;
		}
		
		if(is_adx == 1)
		{
			return 1;
		}
	}
	
	return 0;
}