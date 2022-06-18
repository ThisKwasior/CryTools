#include <common.h>

uint16_t change_order_16(const uint16_t n)
{
	return (n << 8) | (n >> 8);
}

uint32_t change_order_32(const uint32_t n)
{
	return (n >> 24) | 
		   ((n & 0xFF0000) >> 8) |
		   ((n & 0xFF00) << 8) |
		   (n << 24);
}

uint64_t change_order_64(const uint64_t n)
{
	return (n >> 56) | 
		   ((n & 0xFF000000000000) >> 40) |
		   ((n & 0xFF0000000000) >> 24) |
		   ((n & 0xFF00000000) >> 8) |
		   ((n & 0xFF00) << 40) |
		   ((n & 0xFF0000) << 24) |
		   ((n & 0xFF000000) << 8) |
		   (n << 56);
}