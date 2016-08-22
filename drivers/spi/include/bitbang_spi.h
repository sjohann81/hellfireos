/* file:          bitbang_spi.h
 * description:   low level SPI driver
 * date:          09/2015
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

/* SPI mode: 0 ~ 3 */
#define SPI_MODE	0

/* SPI interface - EXTIO_OUT pins 0 (chip select), 1 (clock), 2 (master output) and 3 (chip select 2) */
#define SPI_CS		0x01
#define SPI_SCK		0x02
#define SPI_MOSI	0x04
#define SPI_CS2		0x08

/* SPI interface - EXIO_IN pin 3 (master input) */
#define SPI_MISO	0x08

void spi_setup(void);
void spi_start(void);
void spi_stop(void);
int8_t spi_sendrecv(int8_t data);
