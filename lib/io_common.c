#include <io_common.h>

/*
	Reading
*/

uint8_t io_read_uint8(const uint8_t* data, uint64_t* data_pos)
{
	uint8_t number = *((uint8_t*)&data[*data_pos]);
	*data_pos += 1;
	
	return number;
}

uint16_t io_read_uint16(const uint8_t* data, uint64_t* data_pos)
{
	uint16_t number = *((uint16_t*)&data[*data_pos]);
	*data_pos += 2;
	
	return number;
}

uint32_t io_read_uint32(const uint8_t* data, uint64_t* data_pos)
{
	uint32_t number = *((uint32_t*)&data[*data_pos]);
	*data_pos += 4;
	
	return number;
}

uint64_t io_read_uint64(const uint8_t* data, uint64_t* data_pos)
{
	uint64_t number = *((uint64_t*)&data[*data_pos]);
	*data_pos += 8;

	return number;
}

float io_read_float(const uint8_t* data, uint64_t* data_pos)
{
	float number = *((float*)&data[*data_pos]);
	*data_pos += sizeof(float);

	return number;
}

double io_read_double(const uint8_t* data, uint64_t* data_pos)
{
	double number = *((double*)&data[*data_pos]);
	*data_pos += sizeof(double);

	return number;
}

uint8_t* io_read_array(const uint8_t* data, const uint64_t data_size, uint64_t* data_pos)
{
	uint8_t* array = (uint8_t*)calloc(data_size, 1);
	
	if(array == NULL)
	{
		return 0;
	}

	memcpy(array, &data[*data_pos], data_size);

	*data_pos += data_size;

	return array;
}

/*
	Writing
*/

void io_write_uint8(const uint8_t number, uint8_t* data, uint64_t* data_pos)
{
	data[*data_pos] = number;
	*data_pos += 1;
}

void io_write_uint16(const uint16_t number, uint8_t* data, uint64_t* data_pos)
{
	memcpy(&data[*data_pos], &number, 2);
	*data_pos += 2;
}

void io_write_uint32(const uint32_t number, uint8_t* data, uint64_t* data_pos)
{
	memcpy(&data[*data_pos], &number, 4);
	*data_pos += 4;
}

void io_write_uint64(const uint64_t number, uint8_t* data, uint64_t* data_pos)
{
	memcpy(&data[*data_pos], &number, 8);
	*data_pos += 8;
}

void io_write_float(const float number, uint8_t* data, uint64_t* data_pos)
{
	memcpy(&data[*data_pos], &number, sizeof(float));
	*data_pos += sizeof(float);
}

void io_write_double(const double number, uint8_t* data, uint64_t* data_pos)
{
	memcpy(&data[*data_pos], &number, sizeof(double));
	*data_pos += sizeof(double);
}

void io_write_array(const uint8_t* array, const uint64_t array_size, uint8_t* data, uint64_t* data_pos)
{
	memcpy(&data[*data_pos], array, array_size);
	*data_pos += array_size;
}