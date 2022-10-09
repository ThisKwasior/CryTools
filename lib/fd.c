#include <fd.h>

void file_desc_destroy(File_desc* files, const uint8_t files_amount)
{
	for(uint8_t i = 0; i != files_amount; ++i)
	{
		fclose(files[i].f);
	}
	
	free(files);
}