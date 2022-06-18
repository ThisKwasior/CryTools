#include <io_common.h>

/*
	Reading
*/

void io_read_bits(const uint8_t* data, uint8_t* buffer, const uint32_t buffer_start_bit,
				  const uint32_t data_start_bit, const uint32_t data_bits_to_read)
{
	uint32_t buffer_current_bit = buffer_start_bit;
	uint32_t buffer_bit = buffer_current_bit%8;
	uint32_t buffer_index = (buffer_current_bit-buffer_bit)/8;
	
	uint32_t data_current_bit = data_start_bit;
	uint32_t data_bit = data_current_bit%8;
	uint32_t data_index = (data_current_bit-data_bit)/8;
	
	//printf("%u %u | %u %u\n", buffer_index, buffer_bit, data_index, data_bit);
	
	for(uint32_t i = 0; i != data_bits_to_read; ++i)
	{
		const uint8_t read_bit = (data[data_index]&(1<<(7-data_bit)))>>(7-data_bit);
		buffer[buffer_index] |= read_bit<<buffer_bit;
		
		//printf("%u %u | %u %u | %u\n", buffer_index, buffer_bit, data_index, data_bit, read_bit);
		
		buffer_current_bit+=1;
		buffer_bit = buffer_current_bit%8;
		buffer_index = (buffer_current_bit-buffer_bit)/8;
		
		data_current_bit+=1;
	    data_bit = data_current_bit%8;
	    data_index = (data_current_bit-data_bit)/8;
	}

}

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