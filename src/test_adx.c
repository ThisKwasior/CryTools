#include <stdio.h>
#include <stdlib.h>

#include <adx.h>

int main(int argc, char** argv)
{
	if(argc == 1) return 0;
	
	//FILE* adxFile = fopen(argv[1], "rb");
	ADX adx;
	adx = adx_read_info(argv[1]);
	
	if(adx.info.magic == 0xFFFF)
	{
		printf("This is not an ADX file.\n");
		return 0;
	}
	
	printf("======ADX INFO======\n");
	printf("Copyright offset: %hu/0x%x\n", adx.info.copyright_offset, adx.info.copyright_offset);
	printf("Encoding type: %u\n", adx.info.enc_type);
	printf("Block size: %u\n", adx.info.block_size);
	printf("Bit depth: %u\n", adx.info.bit_depth);
	printf("Channel count: %u\n", adx.info.channel_count);
	printf("Sample rate: %u\n", adx.info.sample_rate);
	printf("Sample count: %u\n", adx.info.sample_count);
	printf("Highpass frequency: %hu\n", adx.info.highpass_freq);
	printf("Version: %u\n", adx.info.version);
	printf("Flags: %u\n", adx.info.flags);
	printf("Estimated length: %fs\n", adx.est_length);
	
	printf("\nLoop enabled: %u\n", adx.loop.loop_flag);
	if(adx.loop.loop_flag)
	{
		printf("Loop start sample: %u\n", adx.loop.loop_start_sample);
		printf("Loop start byte: %u\n", adx.loop.loop_start_byte);
		printf("Loop end sample: %u\n", adx.loop.loop_end_sample);
		printf("Loop end byte: %u\n", adx.loop.loop_end_byte);
	}
	
	return 0;
}