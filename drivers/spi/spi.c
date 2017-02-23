/* file:          spi.c
 * description:   low level SPI driver
 * date:          10/2016
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

#include <hellfire.h>
#include <spi.h>

static uint32_t chip_select, port_mode;

void spi_setup(uint32_t select, uint8_t mode)
{
	uint32_t tmp;

	chip_select = select;
	port_mode = mode;

	tmp = _port_read(SPI_OUTPORT);
	tmp |= chip_select;
	_port_write(SPI_OUTPORT, tmp);

	if (port_mode == 0 || port_mode == 1){
		tmp = _port_read(SPI_OUTPORT);
		tmp &= ~SPI_SCK;
		_port_write(SPI_OUTPORT, tmp);
	}else{
		tmp = _port_read(SPI_OUTPORT);
		tmp |= SPI_SCK;
		_port_write(SPI_OUTPORT, tmp);
	}
}

void spi_start(void)
{
	uint32_t tmp;
	
	tmp = _port_read(SPI_OUTPORT);
	tmp &= ~chip_select;
	_port_write(SPI_OUTPORT, tmp);
}

void spi_stop(void)
{
	uint32_t tmp;
	
	tmp = _port_read(SPI_OUTPORT);
	tmp |= chip_select;
	_port_write(SPI_OUTPORT, tmp);
}

int8_t spi_sendrecv(int8_t data)
{
	int32_t i;
	uint32_t tmp;
	int8_t newdata = 0;

	switch (port_mode){
	case 0:
		for (i = 0; i < 8; i++){
			if (data & 0x80){
				tmp = _port_read(SPI_OUTPORT);
				tmp |= SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}else{
				tmp = _port_read(SPI_OUTPORT);
				tmp &= ~SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}
			data <<= 1;
			
			tmp = _port_read(SPI_OUTPORT);
			tmp |= SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
			
			newdata <<= 1;
			newdata |= ((_port_read(SPI_INPORT) & SPI_MISO) ? 1 : 0);
			
			tmp = _port_read(SPI_OUTPORT);
			tmp &= ~SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
		}
		break;
	case 1:
		for (i = 0; i < 8; i++){
			tmp = _port_read(SPI_OUTPORT);
			tmp |= SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
			
			if (data & 0x80){
				tmp = _port_read(SPI_OUTPORT);
				tmp |= SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}else{
				tmp = _port_read(SPI_OUTPORT);
				tmp &= ~SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}
			data <<= 1;
			
			tmp = _port_read(SPI_OUTPORT);
			tmp &= ~SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
			
			newdata <<= 1;
			newdata |= ((_port_read(SPI_INPORT) & SPI_MISO) ? 1 : 0);
		}
		break;
	case 2:
		for (i = 0; i < 8; i++){
			if (data & 0x80){
				tmp = _port_read(SPI_OUTPORT);
				tmp |= SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}else{
				tmp = _port_read(SPI_OUTPORT);
				tmp &= ~SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}
			data <<= 1;
			
			tmp = _port_read(SPI_OUTPORT);
			tmp &= ~SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
			
			newdata <<= 1;
			newdata |= ((_port_read(SPI_INPORT) & SPI_MISO) ? 1 : 0);
			
			tmp = _port_read(SPI_OUTPORT);
			tmp |= SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
		}
		break;
	case 3:
		for (i = 0; i < 8; i++){
			tmp = _port_read(SPI_OUTPORT);
			tmp &= ~SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
			
			if (data & 0x80){
				tmp = _port_read(SPI_OUTPORT);
				tmp |= SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}else{
				tmp = _port_read(SPI_OUTPORT);
				tmp &= ~SPI_MOSI;
				_port_write(SPI_OUTPORT, tmp);
			}
			data <<= 1;
			
			tmp = _port_read(SPI_OUTPORT);
			tmp |= SPI_SCK;
			_port_write(SPI_OUTPORT, tmp);
			
			newdata <<= 1;
			newdata |= ((_port_read(SPI_INPORT) & SPI_MISO) ? 1 : 0);
		}
		break;
	default:
		break;
	}

	return newdata;
}
