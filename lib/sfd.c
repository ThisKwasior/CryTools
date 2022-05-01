#include "sfd.h"

const uint8_t sofdec2_header[] = "SofdecStream2           ";
const uint8_t sfdmux_info[] = "Created using sfdmux https://github.com/ThisKwasior/CryTools";

void sfd_sofdec2_mpeg_packet(File_desc* fd, const uint8_t fd_size)
{
	uint8_t* buffer = (uint8_t*)calloc(2048, 1);
	
	const uint16_t sofdec_size = 2030;
	const uint16_t sofdec_size_be = change_order_16(sofdec_size);
	const uint16_t info_size = (uint16_t)sizeof(sfdmux_info);
	const uint16_t info_size_be = change_order_16(info_size);
	const uint32_t mpeg_ps_priv2 = change_order_32(PS_PRIV2);
	const uint64_t data_size = 6 + sofdec_size + 6 + info_size;
	
	/*
		metadata packet prep
	*/
	memcpy(&buffer[0], &mpeg_ps_priv2, sizeof(mpeg_ps_priv2));
	memcpy(&buffer[4], &sofdec_size_be, sizeof(sofdec_size_be));
	buffer[6] = 0x08;
	memcpy(&buffer[20], sofdec2_header, sizeof(sofdec2_header));
	
	/* 
		Overall metadata
		TODO: Don't hack this like that please
			  Figure out these structures.
			  There are 3 total:
					initial one (0xAE/0x??)
					audio (0x1AE/x010)
					video (0x3AE/0x40)
					
		We first add some header after sofdecstream string
		then add video at the 0x26 and at 0xAE
		(we need to check the amount of video streams beforehand pls)
	*/

	/* Some kind of header, doesnt really change that much? */
	const uint8_t bytes_after_sf2[4] = {0x02, 0x03, 0x02, 0x49}; 
	memcpy(&buffer[44], bytes_after_sf2, 4);

	/* Sum of streams, audio streams and video streams */
	buffer[0xAE + 6] = fd_size-2+1;
	buffer[0xAE + 7] = fd_size-2;
	buffer[0xAE + 8] = fd_size-1;

	for(uint8_t i = 2; i != fd_size; ++i)
	{
		const uint8_t id = fd[i].stream_id&0xFF;
		const uint16_t record_pos = 0x1AE + 6 + (i-2)*16;
		buffer[record_pos] = id;
		
		if(fd[i].is_adx)
		{
			buffer[record_pos + 1] = 0x21;
		}
		
		if(fd[i].is_aix)
		{
			buffer[record_pos + 1] = 0x23;
		}
		
		// Channel count in first 4 bits
		buffer[record_pos + 2] = (fd[i].channel_count<<4);
		
		// Sample rate
		const uint16_t sample_rate = change_order_16(fd[i].sample_rate);
		memcpy(&buffer[record_pos + 3], &sample_rate, sizeof(uint16_t));
	}
	
	fwrite(buffer, 0x7F4, 1, fd[0].f);
	
	/*
		Padding
	*/
	mpeg_write_padding(fd[0].f, 6);
	
	/*
		sfdmux info
		url to repo etc
	*/
	fwrite(&mpeg_ps_priv2, sizeof(mpeg_ps_priv2), 1, fd[0].f);
	fwrite(&info_size_be, sizeof(info_size_be), 1, fd[0].f);
	fwrite(&sfdmux_info, sizeof(sfdmux_info), 1, fd[0].f);

	/*
		Padding again
	*/
	mpeg_write_padding(fd[0].f, MPEG_MAX_DATA_SIZE-info_size-11);
	
	free(buffer);
}