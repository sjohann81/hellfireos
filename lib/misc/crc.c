#include <hal.h>
#include <libc.h>

/*
crc16 implementation:
width=16 poly=0x1021 init=0xffff refin=false refout=false xorout=0x0000 check=0x29b1 name="CRC-16/CCITT-FALSE"
*/
uint16_t hf_crc16(int8_t *data, uint32_t len){
	uint16_t crc = 0xffff, i;

	while(len--){ 
		crc ^= (uint16_t)*data++ << 8; 

		for(i = 0; i < 8; ++i) 
			crc = crc << 1 ^ (crc & 0x8000 ? 0x1021 : 0x0000); 
	}
	return crc;
}

/*
crc32 implementation:
width=32 poly=0x04c11db7 init=0xffffffff refin=false refout=false xorout=0x00000000 check=0x0376e6e7 name="CRC-32/MPEG-2"
*/
uint32_t hf_crc32(int8_t *data, uint32_t len){
	uint32_t crc = ~0, i;

	while(len--){
		crc ^= (uint32_t)*data++ << 24; 

		for(i = 0; i < 8; ++i)
			crc = crc << 1 ^ (crc & 0x80000000 ? 0x04c11db7 : 0x00000000); 
	}
	return crc;
}

/*
crc64 implementation:
width=64 poly=0x42f0e1eba9ea3693 init=0x0000000000000000 refin=false refout=false xorout=0x0000000000000000 check=0x6c40df5f0b497347 name="CRC-64"
*/
uint64_t hf_crc64(int8_t *data, uint32_t len){
	uint64_t crc = 0;
	uint32_t i;

	while(len--){
		crc ^= (uint64_t)*data++ << 56; 

		for(i = 0; i < 8; ++i)
			crc = crc << 1 ^ (crc & 0x8000000000000000 ? 0x42f0e1eba9ea3693 : 0x0000000000000000); 
	}
	return crc;
}
