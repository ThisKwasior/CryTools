#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <mpeg.h>
#include <utils.h>

int main(int argc, char** argv)
{	
	if(argc == 1) return 0;

	struct Mpeg1Frame frame;
	uint64_t cur_scr = 0;
	
	FILE* sfd = fopen(argv[1], "rb");

	while(fgetc(sfd) != EOF)
	{
		fseek(sfd, -1, SEEK_CUR);
		mpeg_get_next_frame(sfd, &frame);
		
		printf("Sync: %x\n", frame.sync);
		printf("\tLen: %x\n", frame.len);
		printf("\tFDS: %u\n", frame.frame_data_size);
		printf("\tStr: %u\n", frame.stream);
		printf("\tAVi: %u\n", frame.av_id);
		
		uint8_t* scr_start = 0;

		switch(frame.stream)
		{
			case 0xFFFFFFFF:
				scr_start = &frame.data[4];
				
				printf("\tSCR: %u\n", frame.last_scr);
				break;
				
			case STREAM_AUDIO:
				
				printf("\tSCR: %u\n", frame.last_scr);
				
				if(frame.is_adx)
					printf("\tIS ADX\n");
				
				break;
				
			case STREAM_VIDEO:
				
				printf("\tSCR: %u\n", frame.last_scr);

				break;
		}
		
		printf("\tftell: %x\n", ftell(sfd));
	}
	
	fclose(sfd);
}