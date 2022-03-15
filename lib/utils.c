#include <utils.h>

void print_binary(uint8_t* arr, uint32_t size)
{
	printf("Binary: ");
	
	for(uint32_t i = 0; i != size; ++i)
	{
		for(int8_t it = 7; it != -1; --it)
		{
			if(((arr[i]>>it)&1) > 0)
			{
				printf("1");
			}
			else
			{
				printf("0");
			}
		}
		printf(" ");
	}
	printf("\n");
}