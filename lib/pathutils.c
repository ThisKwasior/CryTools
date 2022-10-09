#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pathutils.h"

void split_path(const uint8_t* file_path, const uint16_t file_path_size, File_path_s* desc)
{
	uint16_t ogsize;
	
	if(file_path_size == 0)
	{
		ogsize = strlen(file_path) - 1;
	}
	else
	{
		ogsize = file_path_size;
	}
	
	desc->absolute_dir.size = PATH_NO_VALUE;
	desc->file_name.size = PATH_NO_VALUE;
	desc->ext.size = PATH_NO_VALUE;
	
	int32_t period = PATH_NO_VALUE;
	uint16_t slash = PATH_NO_VALUE;
	
	for(uint16_t i = 0; i != ogsize; ++i)
	{		
		switch(file_path[i])
		{
			case '.':
				period = i;
			break;
			
			case '/':
				slash = i;
			break;
			
			case '\\':
				slash = i;
			break;
		}
	}

	// Absolute
	
	if(slash != PATH_NO_VALUE)
	{
		desc->absolute_dir.size = slash + 1; // We need to include the slash
		desc->absolute_dir.ptr = calloc(desc->absolute_dir.size+1, sizeof(char));
		strncpy(desc->absolute_dir.ptr, &file_path[0], desc->absolute_dir.size);
		desc->absolute_dir.ptr[desc->absolute_dir.size] = 0;
	}
	
	// Name

	if(slash == PATH_NO_VALUE)
	{
		slash = 0;
	}
	else
	{
		++slash;
	}
	
	if(period == PATH_NO_VALUE)
	{
		period = ogsize;
	}

	desc->file_name.size = period - slash;
	desc->file_name.ptr = calloc(desc->file_name.size+1, sizeof(char));
	strncpy(desc->file_name.ptr, &file_path[slash], desc->file_name.size);
	desc->file_name.ptr[desc->file_name.size] = 0;
	
	// Extension
	
	if(period != PATH_NO_VALUE)
	{
		desc->ext.size = ogsize - period;
		desc->ext.ptr = calloc(desc->ext.size+1, sizeof(char));
		strncpy(desc->ext.ptr, &file_path[period+1], desc->ext.size);
		desc->ext.ptr[desc->ext.size] = 0;
	}
}

void free_path(File_path_s* desc)
{
	free(desc->absolute_dir.ptr);
	desc->absolute_dir.size = 0;
	
	free(desc->file_name.ptr);
	desc->file_name.size = 0;
	
	free(desc->ext.ptr);
	desc->ext.size = 0;
}

void change_slashes(char* str, uint16_t size)
{
	while(size)
	{
		if(str[size-1] == '\\')
		{
			str[size-1] = '/';
		}
		
		--size;
	}
}

char* create_string(File_path_s* desc)
{
	// Calculating the size of the string
	
	uint16_t size = 0;

	if(desc->absolute_dir.size != PATH_NO_VALUE)
	{
		size += desc->absolute_dir.size;
	}

	if(desc->file_name.size != PATH_NO_VALUE)
	{
		size += desc->file_name.size;
	}
		
	if(desc->ext.size != PATH_NO_VALUE)
	{
		size += desc->ext.size;
	}
	
	size += 2; // For NULL and period before ext
	
	// Creating the string
	
	char* str = (char*)calloc(size, 1);
	size = 0;
	
	if(desc->absolute_dir.size != PATH_NO_VALUE)
	{
		strncpy(str, desc->absolute_dir.ptr, desc->absolute_dir.size);
		size += desc->absolute_dir.size;
	}

	if(desc->file_name.size != PATH_NO_VALUE)
	{
		strncpy(&str[size], desc->file_name.ptr, desc->file_name.size);
		size += desc->file_name.size;
	}
		
	if(desc->ext.size != PATH_NO_VALUE)
	{
		str[size] = '.';
		strncpy(&str[size+1], desc->ext.ptr, desc->ext.size);
	}
	
	return str;
}

void add_to_string(mut_cstring* dest, mut_cstring* suffix)
{
	const uint16_t size = dest->size + suffix->size + 1;
	
	dest->ptr = (char*)realloc((dest->ptr), size);
	dest->ptr[size-1] = 0;
	strncpy(&dest->ptr[(dest->size)], suffix->ptr, suffix->size);
	
	//printf("New: %s %u %u %u\n", dest->ptr, strlen(dest->ptr), size, suffix->size);
	
	dest->size = size-1;
}

void replace_string(mut_cstring* dest, mut_cstring* replace)
{
	dest->size = replace->size;
	free(dest->ptr);
	dest->ptr = calloc(1, dest->size);
	memccpy(dest->ptr, replace->ptr, 0, dest->size);
}

void replace_raw_string(mut_cstring* dest, const uint8_t* replace, const uint32_t size)
{
	dest->size = size-1;
	free(dest->ptr);
	dest->ptr = calloc(1, size);
	dest->ptr[size-1] = 0;
	memccpy(dest->ptr, replace, 0, dest->size);
}

void print_path(File_path_s desc)
{
	printf("absolute: %s\n", desc.absolute_dir.ptr);
	printf("filename: %s\n", desc.file_name.ptr);	
	printf("extnsion: %s\n", desc.ext.ptr);
}