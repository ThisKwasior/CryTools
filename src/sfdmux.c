#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <adx.h>
#include <mpeg.h>
#include <utils.h>
#include <sfd.h>

/*
	Gives you the amount of frames ADX file will have and the SCR time step.
	Returns the ADX data.
*/
ADX get_adx_frame_info(FILE* adx_file, uint32_t* frame_count, float* time_per_frame);

/*
	Prepares ADX mpeg frame to write
*/
uint8_t* prepare_adx_mpeg_frame(FILE* adx_file, const float adx_global_scr, uint32_t* data_size);

/*
	Prints usage and small doc
*/
void print_doc();

/*
	Muxes video and audio
*/
void mux(const int32_t argc, char** argv);

int main(int argc, char** argv)
{	
	if(argc <= 2)
	{
		print_doc();
		return 0;
	}

	mux(argc, argv);
}

ADX get_adx_frame_info(FILE* adx_file, uint32_t* frame_count, float* time_per_frame)
{
	uint64_t adx_size = 0;
	
	fseek(adx_file, 0, SEEK_END);
	adx_size = ftell(adx_file);
	fseek(adx_file, 0, SEEK_SET);
	
	ADX adx = readADXInfo(adx_file);
	
	puts("");
	printf("Size of ADX: %u | 0x%x\n", adx_size, adx_size);
	
	*frame_count = adx_size/(MPEG_MAX_DATA_SIZE-7);
	
	if(adx_size%(MPEG_MAX_DATA_SIZE-2) != 0)
	{
		*frame_count += 1;
	}
	
	printf("Frame count: %u\n", (*frame_count));
	
	*time_per_frame = adx.estLength/(float)(*frame_count);
	
	printf("Time per frame: %f\n", (*time_per_frame));
	
	return adx;
}

uint8_t* prepare_adx_mpeg_frame(FILE* adx_file, const float adx_global_scr, uint32_t* data_size)
{
	uint8_t* data = (uint8_t*)calloc(1, MPEG_MAX_DATA_SIZE+6);
	
	const uint32_t file_pos_before = ftell(adx_file);
	const uint32_t stream_id = CHGORD32(MPEG_AUDIO_START_ID + 0);
	const uint64_t scr_to_encode = adx_global_scr*MPEG_SCR_MUL;
	uint8_t scr[5] = {0};
	mpeg1_encode_scr(scr, scr_to_encode);

	memcpy(&data[0], &stream_id, 4);
	memcpy(&data[6], &adx_frame_value, 2);
	memcpy(&data[8], scr, 5);
	
	fread(&data[13], MPEG_MAX_DATA_SIZE-7, 1, adx_file);
	const uint32_t file_pos_after = ftell(adx_file);
	const uint16_t bytes_read = file_pos_after - file_pos_before;
	
	// +7 because 0x4004 and 5bytes of SCR
	const uint16_t bytes_read_BE = CHGORD16(bytes_read+7);
	
	memcpy(&data[4], &bytes_read_BE, 2);

	*data_size = bytes_read;
	
	*data_size += 13;
	
	return data;
}

void mux(const int32_t argc, char** argv)
{
	uint32_t adx_frame_count = 0;
	float adx_scr_step = 0;
	float adx_global_scr = 0;
	float mpeg_global_scr = 0;

	FILE* adxf = fopen(argv[1], "rb");
	FILE* mpegf = fopen(argv[2], "rb");
	FILE* sfdf;
	
	FILE* logf = fopen("log.txt", "wb");
	
	if(argc == 4)
		sfdf = fopen(argv[3], "wb");
	else
		sfdf = fopen("movie.sfd", "wb");
	
	/* Init mpeg info */
	struct Mpeg1Frame mpeg_frame;
	memset(&mpeg_frame, 0, sizeof(struct Mpeg1Frame));
	
	/* ADX stuff */
	printf("Writting SFD header\n");
	fwrite(sofdecHeader, sizeof(sofdecHeader), 1, sfdf);
	
	ADX adx = get_adx_frame_info(adxf, &adx_frame_count, &adx_scr_step);

	/* Current characters from the files */
	uint32_t adxc = 0;
	uint32_t mpegc = 0;
	
	/* 
		Had to set it to this instead of 0.
		Otherwise video is not synced.
		Decoder starved and can't keep up?
	*/
	adx_global_scr += adx_scr_step*10;

	while(1)
	{
		/* Getting next byte from the files */
		adxc = fgetc(adxf);
		mpegc = fgetc(mpegf);
		
		if(adxc != EOF) 
			fseek(adxf, -1, SEEK_CUR);
		
		if(mpegc != EOF) 
			fseek(mpegf, -1, SEEK_CUR);
		
		if((adxc == EOF) && (mpegc == EOF)) break;
		
		if((adxc != EOF && adx_global_scr <= mpeg_global_scr) || (mpegc == EOF))
		{
			adx_global_scr += adx_scr_step;
			uint32_t data_size;
			const uint8_t* data_to_write = prepare_adx_mpeg_frame(adxf, adx_global_scr, &data_size);
			fwrite(data_to_write, data_size, 1, sfdf);
			fprintf(logf, "Writting ADX | %f < %f\n", adx_global_scr, mpeg_global_scr);
		}
		
		if(mpegc != EOF)
		{
			mpeg_get_next_frame(mpegf, &mpeg_frame);
			mpeg_global_scr = mpeg_frame.last_scr/(float)MPEG_SCR_MUL;
			fwrite(mpeg_frame.data, mpeg_frame.len, 1, sfdf);
			fprintf(logf, "Video: %d | %f | %f\n", mpeg_frame.stream, mpeg_global_scr, adx_global_scr);
		}
	}
	
	/* Mpeg program end */

	const uint8_t pe[] = {0, 0, 1, 0xB9};
	fwrite(pe, sizeof(pe), sizeof(uint8_t), sfdf);
	
	for(uint32_t i = 0; i != 2044; ++i)
	{
		fputc(0xFF, sfdf);
	}

	/* Closing file handles */
	fclose(adxf);
	fclose(sfdf);
	fclose(mpegf);
	fclose(logf);
}

void print_doc()
{
	printf("===========SFDMUX BETA===========\n\n");
	printf("========USAGE========\n");
	printf("\n");	
	printf("\tsfdmux.exe <adx file> <mpeg file> <output file>\n");
	printf("\n");		
	printf("\tADX \t- Standard CRI ADPCM audio.\n");
	printf("\n");	
	printf("\tMPEG\t- MPEG-PS Video without audio streams.\n");
	printf("\t\t  Audio streams can be present, but the\n");
	printf("\t\t  compatibility with games can be low.\n");
	printf("\n");			  
	printf("\tOUTPUT\t- Muxed file. Can be ommited.\n");
	printf("\t\t  If ommited, all work is saved to \"movie.sfd\".\n");
	printf("\n");
	printf("========ENCODING TUTORIAL========\n");
	printf("\n");
	printf("\tWe'll be using FFmpeg for this.\n");
	printf("\n");
	printf("\tFor fast and dirty SFD we need to reencode the source\n");
	printf("\tto mpeg1video and adpcm_adx.\n");
	printf("\tHere's as follows:\n");
	printf("\n");
	printf("\t\tffmpeg -i video.webm -an -c:v mpeg1video video.mpeg\n");
	printf("\n");
	printf("\tResulting video will have no audio streams and bitrate suggested by FFmpeg. \n");
	printf("\tChange bitrate to your needs.\n");
	printf("\tAlso some games can be picky about resolutions, change that too if\n");
	printf("\tthe video skips or game crashes.\n");
	printf("\n");
	printf("\tI also recommend 2pass encoding:\n");
	printf("\n");
	printf("\t\tffmpeg -i vid.webm -an -c:v mpeg1video -b:v 8M -pass 1 -f mpeg NUL\n");
	printf("\t\tffmpeg -i vid.webm -an -c:v mpeg1video -b:v 8M -pass 2 -f mpeg video.mpeg\n");
	printf("\n");
	printf("\tFor Linux, change NUL to /dev/null\n");
	printf("\n");
	printf("\tNow for audio:\n");
	printf("\n");
	printf("\t\tffmpeg -i video.webm -vn audio.adx\n");
	printf("\n");	
	printf("\tResulting ADX will be of nice quality, but for greater control over this\n");
	printf("\tI recommend VGAudio.\n");
	printf("\n");
	printf("\t\thttps://github.com/Thealexbarney/VGAudio\n");
	printf("\n");	
	printf("\tAfter all of this we can finally mux.\n");
	printf("\n");
	printf("\t\tsfdmux.exe audio.adx video.mpeg movie.sfd\n");
	printf("\n");
	printf("========DISCLAIMER========\n");
	printf("\n");
	printf("\tThis program is only intended for muxing audio and video.\n");
	printf("\tThis means it does not encode anything.\n");
	printf("\tQuality of the video/audio is dependent on you only.\n");
	printf("\n");
	printf("\tProgram is provided as-is blah blah blah.\n");
	printf("\tDon't sue me over fried PCs or something.\n");
	printf("\n");
	printf("========CREDITS========\n");
	printf("\n");
	printf("\tProgrammer \t- Kwasior\n");
	printf("\t\t\t  https://github.com/ThisKwasior\n");
	printf("\t\t\t  https://twitter.com/ThisKwasior\n");
}
