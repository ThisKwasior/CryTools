#pragma once

#include <stdint.h>
#include <stdio.h>

#include "utils.h"

/*
	Source: https://en.wikipedia.org/wiki/ADX_(file_format)#Technical_Description
*/

const static uint8_t adx_frame_value[] = {0x40, 0x04};

enum EncodingType
{
	COEFF = 0x02,
	STANDARD = 0x03,
	EXP_SCALE = 0x04,
	AHX1 = 0x10,
	AHX2 = 0x11,
};

struct AdxBasicInfo
{
	uint16_t magic; // Should always be LE 0x0080
	uint16_t copyrightOffset; // "(c)CRI" string at [copyrightOffset-2] 
	uint8_t encType; // Encoding type
	uint8_t blockSize;
	uint8_t bitdepth;
	uint8_t channelCount;
	uint32_t sampleRate;
	uint32_t sampleCount;
	uint16_t highpassFreq;
	uint8_t version;
	uint8_t flags;
	uint8_t loopEnabled;
};

struct AdxLoop
{
	
};

typedef struct Adx
{
	struct AdxBasicInfo info;
	float estLength;
	
} ADX;

ADX readADXInfo(FILE* adxFilePtr);