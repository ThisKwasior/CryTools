#include <stdio.h>
#include <stdint.h>

//360256593 decoded
static const unsigned char scr[5] = {0x21, 0x55, 0xE5, 0x28, 0xA3};

/*
    We're leaving mux rate out since i don't know
    how to use it
*/
uint64_t decode_scr(const unsigned char* arr)
{
    uint64_t decoded = 0;
    
    uint64_t first      = (arr[0]&14)>>1;
    uint64_t second     = arr[1];
    uint64_t third      = (arr[2]&254)>>1;
    uint64_t fourth     = arr[3];
    uint64_t fifth      = (arr[4]&254)>>1;

    decoded |= (first<<30);
    decoded |= (second<<22);
    decoded |= (third<<15);
    decoded |= (fourth<<7);
    decoded |= fifth;
    
    return decoded;
}

void encode_scr(unsigned char* arr, const uint64_t scr_value)
{
	uint64_t buf = 0;
	
	arr[0] |= 33;
	buf = scr_value>>29;
	arr[0] |= buf;
	
	buf = (scr_value>>22)&0xFF;
	arr[1] |= buf;
	
	arr[2] |= 1;
	buf = (scr_value>>14)&254;
	arr[2] |= buf;
	
	buf = (scr_value>>7)&0xFF;
	arr[3] |= buf;
	
	arr[4] |= 1;
	buf = (scr_value<<1)&254;
	arr[4] |= buf;
}

int main()
{
	unsigned char encoded[5] = {0};
	
    printf("%u\n", decode_scr(scr));
	encode_scr(&encoded, 0);
	//printf("%x %x %x %x %x\n", encoded[0], encoded[1], encoded[2], encoded[3], encoded[4]);

    return 0;
}