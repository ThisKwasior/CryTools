#include "sfd.h"

const uint8_t sofdec2_header[] = "SofdecStream2           ";
const uint8_t sfdmux_info[] = "Created using sfdmux https://github.com/ThisKwasior/CryTools";

void sfd_sofdec2_mpeg_packet(File_desc* fd, const uint8_t fd_size)
{
	const uint16_t sofdec_size = 2030;
	const uint16_t sofdec_size_be = change_order_16(sofdec_size);
	const uint16_t info_size = (uint16_t)sizeof(sfdmux_info);
	const uint16_t info_size_be = change_order_16(info_size);
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint64_t data_size = 6 + sofdec_size + 6 + info_size;

	fseek(fd[0].f, 0, SEEK_SET);
	
	/*
		metadata packet prep
	*/
	fwrite(&mpeg_ps_priv2, sizeof(mpeg_ps_priv2), 1, fd[0].f);
	fwrite(&sofdec_size_be, sizeof(sofdec_size_be), 1, fd[0].f);
	fputc(0x08, fd[0].f);
	fseek(fd[0].f, 20, SEEK_SET);
	fwrite(&sofdec2_header, sizeof(sofdec2_header)-1, 1, fd[0].f);
	
	/* 
		Overall metadata
		TODO: Don't hack this like that please
			  Figure out these structures.
			  There are 3 total:
					initial one (0xAE/0x??)
					audio (0x1AE/x010)
					video (0x3AE/0x40)
					
		We first add the amount of audio streams, then video
		at the 0x26 and at 0xAE
		(we need to check the amount of video streams beforehand pls)
	*/

	/* Audio streams and video streams */
	fseek(fd[0].f, 6 + 0x26, SEEK_SET);
	fputc(fd_size-2, fd[0].f);
	fputc(1, fd[0].f);

	/* Sum of streams, audio streams and video streams */
	fseek(fd[0].f, 6 + 0xAE, SEEK_SET);
	fputc(fd_size-2 + 1, fd[0].f);
	fputc(fd_size-2, fd[0].f);
	fputc(1, fd[0].f);

	fseek(fd[0].f, 6 + 0x1AE, SEEK_SET);
	
	for(uint8_t i = 2; i != fd_size; ++i)
	{
		const uint8_t id = fd[i].stream_id&0xFF;
		fputc(id, fd[0].f);
		fputc(0x21, fd[0].f);
		fputc(0x20, fd[0].f);
		fseek(fd[0].f, 0x0D, SEEK_CUR);
	}
	
	/*
		sfdmux info
		url to repo etc
	*/
	fseek(fd[0].f, 6 + sofdec_size, SEEK_SET);
	fwrite(&mpeg_ps_priv2, sizeof(mpeg_ps_priv2), 1, fd[0].f);
	fwrite(&info_size_be, sizeof(info_size_be), 1, fd[0].f);
	fwrite(&sfdmux_info, sizeof(sfdmux_info), 1, fd[0].f);
}