/* file:          eeprom25lcxx.c
 * description:   Microchip EEPROM chip driver
 * date:          09/2015
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 * 
 * all EEPROMs compatible with the Microchip 25lcxx family.
 * page size should be set according to EEPROM chip in use:
 * 25xx08, 25xx16: 16
 * 25xx32, 25xx64: 32
 * 25xx128, 25xx256: 64
 * 25xx512, 25xx1024: 128
 */

#include <hellfire.h>
#include <spi.h>
#include <eeprom25lcxx.h>

uint8_t eeprom25lcxx_readbyte(uint16_t addr)
{
	uint8_t data;
	
	spi_start();
	spi_sendrecv(CMD_READ);
	spi_sendrecv(addr >> 8);
	spi_sendrecv(addr & 0xff);
	data = spi_sendrecv(0);
	spi_stop();
	
	return data;
}

void eeprom25lcxx_read(uint16_t addr, uint8_t *buf, uint16_t size)
{
	uint16_t i;
	
	spi_start();
	spi_sendrecv(CMD_READ);
	spi_sendrecv(addr >> 8);
	spi_sendrecv(addr & 0xff);
	for(i = 0; i < size; i++)
		buf[i] = spi_sendrecv(0);
	spi_stop();
}

void eeprom25lcxx_writepage(uint16_t page, uint8_t page_size, uint8_t *data)
{
	uint16_t i;
	
	spi_start();
	spi_sendrecv(CMD_WREN);
	spi_stop();
	spi_start();
	spi_sendrecv(CMD_WRITE);
	spi_sendrecv((page * page_size) >> 8);
	spi_sendrecv((page * page_size) & 0xff);
	for (i = 0; i < page_size; i++)
		spi_sendrecv(data[i]);
	spi_stop();
	delay_ms(10);
}
