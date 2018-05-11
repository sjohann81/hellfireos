/* file:          sram23lcxx.c
 * description:   Microchip SRAM chip driver
 * date:          12/2016
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 * 
 * compatible with the 23lc512 and 23lc1024 Microchip family chips.
 * this driver controls the chip using the SPI interface and operates
 * in sequential mode only. the 23lc1024 should be accessed using
 * hiaddr set to 1, other chips use 0.
 * this driver will not work directly with 23x256 chips, because they
 * don't operate in sequential mode by default.
 */


#include <hellfire.h>
#include <spi.h>
#include <sram23lcxx.h>

void sram25lcxx_read(uint32_t addr, uint8_t hiaddr, uint8_t *buf, uint32_t size)
{
	uint32_t i;
	
	spi_start();
	spi_sendrecv(CMD_READ);
	if (hiaddr)
		spi_sendrecv((addr >> 16) & 0xff);
	spi_sendrecv((addr >> 8) & 0xff);
	spi_sendrecv(addr & 0xff);
	for(i = 0; i < size; i++)
		buf[i] = spi_sendrecv(0);
	spi_stop();
}

void sram25lcxx_write(uint32_t addr, uint8_t hiaddr, uint8_t *buf, uint32_t size)
{
	uint32_t i;
	
	spi_start();
	spi_sendrecv(CMD_WRITE);
	if (hiaddr)
		spi_sendrecv((addr >> 16) & 0xff);
	spi_sendrecv((addr >> 8) & 0xff);
	spi_sendrecv(addr & 0xff);
	for(i = 0; i < size; i++)
		spi_sendrecv(buf[i]);
	spi_stop();
}
