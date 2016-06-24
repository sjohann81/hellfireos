#define NOC_READ			0x20000070	/*READ*/
#define NOC_WRITE			0x20000080	/*WRITE*/
#define NOC_STATUS			0x20000090	/*STATUS*/
#define NOC_CTRL			0x200000C0	/*CONTROL*/

#define IRQ_NOC_READ			0x100

uint16_t _ni_status(void);
uint16_t _ni_read(void);
void _ni_write(uint16_t data);
