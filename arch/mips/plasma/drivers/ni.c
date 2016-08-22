#include <hellfire.h>
#include <ni.h>

uint16_t _ni_status(void)
{
	uint16_t status;

	status = (uint16_t)MemoryRead(NOC_STATUS);
//	asm ("nop\nnop\nnop");
	return status;
}

uint16_t _ni_read(void)
{
	uint16_t data;

	data = (uint16_t)MemoryRead(NOC_READ);
//	asm ("nop\nnop\nnop");
	return data;
}

void _ni_write(uint16_t data)
{
	MemoryWrite(NOC_WRITE, data);
//	asm ("nop\nnop\nnop");
}
