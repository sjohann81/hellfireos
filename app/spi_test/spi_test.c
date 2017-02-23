#include <hellfire.h>
#include <spi.h>

void test(void){
	int i;
	char buf[50] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};
	
	spi_setup(SPI_CS0, 0);
	
	while (1){
		spi_start();
		for (i = 0; i < 10; i++)
			buf[i] = spi_sendrecv(buf[i]);
		spi_stop();
		delay_ms(1);
	}
}

void app_main(void){
	hf_spawn(test, 0, 0, 0, "test thread", 2048);
}
