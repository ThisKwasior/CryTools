#pragma once

#include <stdint.h>

#define PATH_NO_VALUE 0xFFFF

typedef struct
{
	char* ptr;
	uint16_t size;
} mut_cstring;

/*
	Description of the given path.
	
	If the size == 0xFFFF, then the given char* does not exist.
*/
typedef struct File_path_desc
{
	mut_cstring absolute_dir; // ex. C:/projects
	mut_cstring file_name; // ex. pathsplit	
	mut_cstring ext; // ex. `exe`, `h`, `ass`
} File_path_s;

/*
	Splits the path and allocates all arrays in `desc` with proper data.
	
	If `file_path_size` == 0 then it will determine size itself.
*/
void split_path(const uint8_t* file_path, const uint16_t file_path_size, File_path_s* desc);

/*
	Frees all char arrays inside of the `desc`.
	It won't free the `desc` itself.
*/
void free_path(File_path_s* desc);

/*
	Converts any MS-DOS '\' to *nix '/'
*/
void change_slashes(char* str, uint16_t size);

/*
	Creates a null-terminated string from File_path_s content and returns a pointer to it.
*/
char* create_string(File_path_s* desc);

/*
	Adds a string at the end of input with new size.
	Expects for strings to be null terminated.
*/
void add_to_string(mut_cstring* dest, mut_cstring* suffix);

/*
	Replaces a string
*/
void replace_string(mut_cstring* dest, mut_cstring* replace);

/*
	Replaces a raw string
*/
void replace_raw_string(mut_cstring* dest, const uint8_t* replace, const uint32_t size);

/*
	Prints every part of the path (debugging)
*/
void print_path(File_path_s desc);