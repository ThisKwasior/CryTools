#include "adx.h"

ADX readADXInfo(FILE* adxFilePtr)
{
	rewind(adxFilePtr);
	
	ADX adx;
	
	fread(&adx.info, sizeof(struct AdxBasicInfo), 1, adxFilePtr);
	
	adx.info.copyrightOffset = CHGORD16(adx.info.copyrightOffset);
	adx.info.sampleRate = CHGORD32(adx.info.sampleRate);
	adx.info.sampleCount = CHGORD32(adx.info.sampleCount);
	adx.info.highpassFreq = CHGORD16(adx.info.highpassFreq);
	
	if(adx.info.version == 3) fseek(adxFilePtr, 0x1B, SEEK_SET);
	else if(adx.info.version == 4) fseek(adxFilePtr, 0x27, SEEK_SET);

	fread(&adx.info.loopEnabled, sizeof(uint8_t), 1, adxFilePtr);

	adx.estLength = adx.info.sampleCount/(float)adx.info.sampleRate;
	
	/*printf("======ADX INFO======\n");
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
	printf("Estimated length: %fs\n", adx.estLength);*/

	rewind(adxFilePtr);

	return adx;
}