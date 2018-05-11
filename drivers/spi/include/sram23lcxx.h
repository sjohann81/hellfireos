/* file:          sram23lcxx.h
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

#define CMD_READ	0x03
#define CMD_WRITE	0x02
#define CMD_EDIO	0x3b
#define CMD_EQIO	0x38
#define CMD_RSTIO	0xff
#define CMD_RDMR	0x05
#define CMD_WRMR	0x01

void sram25lcxx_read(uint32_t addr, uint8_t hiaddr, uint8_t *buf, uint32_t size);
void sram25lcxx_write(uint32_t addr, uint8_t hiaddr, uint8_t *buf, uint32_t size);
