/* file:          eeprom25lcxx.h
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

#define CMD_READ	0x03
#define CMD_WRITE	0x02
#define CMD_WRDI	0x04
#define CMD_WREN	0x06
#define CMD_RDSR	0x05
#define CMD_WRSR	0x01

uint8_t eeprom25lcxx_readbyte(uint16_t addr);
void eeprom25lcxx_read(uint16_t addr, uint8_t *buf, uint16_t size);
void eeprom25lcxx_writepage(uint16_t page, uint8_t page_size, uint8_t *data);
