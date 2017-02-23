/* file:          spi.h
 * description:   low level SPI driver
 * date:          10/2016
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

void spi_setup(uint32_t select, uint8_t mode);
void spi_start(void);
void spi_stop(void);
int8_t spi_sendrecv(int8_t data);
