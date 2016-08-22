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
#include <eeprom25lcxx.h>

uint8_t spi_eeprom_readbyte(uint16_t addr){
	uint8_t data;
	
	spi_start();
	spi_sendrecv(CMD_READ);
	spi_sendrecv(addr >> 8);
	spi_sendrecv(addr & 0xff);
	data = spi_sendrecv(0);
	spi_stop();
	
	return data;
}

void spi_eeprom_read(uint16_t addr, uint8_t *buf, uint16_t size){
	uint8_t data;
	uint16_t i;
	
	spi_start();
	spi_sendrecv(CMD_READ);
	spi_sendrecv(addr >> 8);
	spi_sendrecv(addr & 0xff);
	for(i = 0; i < size; i++)
		buf[i] = spi_sendrecv(0);
	spi_stop();
}

void spi_eeprom_writepage(uint16_t page, uint8_t *data){
	uint16_t i;
	
	spi_start();
	spi_sendrecv(CMD_WREN);
	spi_stop();
	spi_start();
	spi_sendrecv(CMD_WRITE);
	spi_sendrecv((page * PAGE_SIZE) >> 8);
	spi_sendrecv((page * PAGE_SIZE) & 0xff);
	for (i = 0; i < PAGE_SIZE; i++)
		spi_sendrecv(data[i]);
	spi_stop();
	delay_ms(10);
}
