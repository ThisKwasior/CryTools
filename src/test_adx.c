#include <stdio.h>
#include <stdlib.h>

#include <adx.h>

int main(int argc, char** argv)
{
	if(argc == 1) return 0;
	
	FILE* adxFile = fopen(argv[1], "rb");
	ADX adx;
	adx = readADXInfo(adxFile);
	
	printf("======ADX INFO======\n");
	printf("Copyright offset: %u\n", adx.info.copyrightOffset);
	printf("encType offset: %u\n", adx.info.encType);
	printf("blockSize offset: %u\n", adx.info.blockSize);
	printf("bitdepth offset: %u\n", adx.info.bitdepth);
	printf("channelCount offset: %u\n", adx.info.channelCount);
	printf("sampleRate offset: %u\n", adx.info.sampleRate);
	printf("sampleCount offset: %u\n", adx.info.sampleCount);
	printf("highpassFreq offset: %u\n", adx.info.highpassFreq);
	printf("version offset: %u\n", adx.info.version);
	printf("flags offset: %u\n", adx.info.flags);
	printf("Loop enabled: %u\n", adx.info.loopEnabled);
	printf("Estimated length: %fs\n", adx.estLength);
	
	fclose(adxFile);
	return 0;
}