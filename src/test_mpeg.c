#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <mpeg.h>

int main(int argc, char** argv)
{	
	if(argc == 1) return 0;

	struct Mpeg1Frame frame;
	
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

		switch(frame.stream)
		{
			case 0xFFFFFFFF:
				printf("\tSCR: %llu\n", frame.last_scr);
				break;
				
			case STREAM_AUDIO:
				
				printf("\tSCR: %llu\n", frame.last_scr);
				
				if(frame.is_adx)
					printf("\tIS ADX\n");
				
				break;
				
			case STREAM_VIDEO:
				
				printf("\tSCR: %llu\n", frame.last_scr);

				break;
		}
		
		printf("\tftell: %x\n", ftell(sfd));
	}
	
	fclose(sfd);
}