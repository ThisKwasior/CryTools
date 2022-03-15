#include "adx.h"

ADX read_adx_info(FILE* adx_file)
{
	rewind(adx_file);
	
	ADX adx;
	uint8_t is_adx = 0;
	
	fread(&adx.info, sizeof(struct AdxBasicInfo), 1, adx_file);
	
	if(adx.info.version == 3)
	{
		is_adx += 1;
		fread(&adx.unknown_loop, 4, 1, adx_file);
	}
	else if(adx.info.version == 4)
	{
		is_adx += 1;
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

	/* 
		Checking if the file is indeed ADX.
		If not, magic will be 0xFFFF
	*/
	
	if(adx.info.magic == 0x8000)
	{
		is_adx += 1;
	}
	
	fseek(adx_file, adx.info.copyright_offset-2, SEEK_SET);
	uint8_t cri[6] = {0};
	fread(cri, 6, 1, adx_file);
	
	if(strncmp(cri, "(c)CRI", 6) == 0)
	{
		is_adx += 1;
	}
	
	if(is_adx != 3)
	{
		//printf("Unrecognized ADX file.\n");
		adx.info.magic = 0xFFFF;
	}

	fseek(adx_file, 0, SEEK_END);
	adx.file_size = ftell(adx_file);
	rewind(adx_file);

	return adx;
}