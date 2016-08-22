/* file:          bitbang_spi.c
 * description:   low level SPI driver
 * date:          09/2015
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

#include <hellfire.h>
#include <bitbang_spi.h>

void spi_setup(void){
	EXTIO_OUT |= SPI_CS;

#if SPI_MODE == 0 || SPI_MODE == 1
	EXTIO_OUT &= ~SPI_SCK;
#else
	EXTIO_OUT |= SPI_SCK;
#endif
}

void spi_start(void){
	EXTIO_OUT &= ~SPI_CS
}

void spi_stop(void){
	EXTIO_OUT |= SPI_CS;
}

int8_t spi_sendrecv(int8_t data){
	int32_t i;
	int8_t newdata = 0;

#if SPI_MODE == 0	
	for (i = 0; i < 8; i++){
		if (data & 0x80){
			extio_out |= SPI_MOSI;
			EXTIO_OUT = extio_out;
		}else{
			extio_out &= ~SPI_MOSI;
			EXTIO_OUT = extio_out;
		}
		data <<= 1;
		extio_out |= SPI_SCK;
		EXTIO_OUT = extio_out;
		newdata <<= 1;
		newdata |= ((EXTIO_IN & SPI_MISO) ? 1 : 0);
		extio_out &= ~SPI_SCK;
		EXTIO_OUT = extio_out;
	}
	return newdata;
#endif
#if SPI_MODE == 1
	for (i = 0; i < 8; i++){
		extio_out |= SPI_SCK;
		EXTIO_OUT = extio_out;
		if (data & 0x80){
			extio_out |= SPI_MOSI;
			EXTIO_OUT = extio_out;
		}else{
			extio_out &= ~SPI_MOSI;
			EXTIO_OUT = extio_out;
		}
		data <<= 1;
		extio_out &= ~SPI_SCK;
		EXTIO_OUT = extio_out;
		newdata <<= 1;
		newdata |= ((EXTIO_IN & SPI_MISO) ? 1 : 0);
	}
	return newdata;
#endif
#if SPI_MODE == 2
	for (i = 0; i < 8; i++){
		if (data & 0x80){
			extio_out |= SPI_MOSI;
			EXTIO_OUT = extio_out;
		}else{
			extio_out &= ~SPI_MOSI;
			EXTIO_OUT = extio_out;
		}
		data <<= 1;
		extio_out &= ~SPI_SCK;
		EXTIO_OUT = extio_out;
		newdata <<= 1;
		newdata |= ((EXTIO_IN & SPI_MISO) ? 1 : 0);
		extio_out |= SPI_SCK;
		EXTIO_OUT = extio_out;
	}
	return newdata;
#endif
#if SPI_MODE == 3
	for (i = 0; i < 8; i++){
		extio_out &= ~SPI_SCK;
		EXTIO_OUT = extio_out;
		if (data & 0x80){
			extio_out |= SPI_MOSI;
			EXTIO_OUT = extio_out;
		}else{
			extio_out &= ~SPI_MOSI;
			EXTIO_OUT = extio_out;
		}
		data <<= 1;
		extio_out |= SPI_SCK;
		EXTIO_OUT = extio_out;
		newdata <<= 1;
		newdata |= ((EXTIO_IN & SPI_MISO) ? 1 : 0);
	}
	return newdata;
#endif
}
