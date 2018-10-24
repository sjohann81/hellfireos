#include <hellfire.h>
#include <noc.h>

int32_t callback(uint16_t *buffer)
{
	printf("Hello World! got message: %s\n", buffer + PKT_HEADER_SIZE);
	
	return 0;
}

void thread(void)
{
	int32_t test = 0;
	int8_t buf[100];
	int16_t val;

	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);

	delay_ms(50);

	while (1){
		sprintf(buf, "test %d", test++);
		/* sending data to callback port. message size must small (1 packet) */
		val = hf_send(1, 0xffff, buf, sizeof(buf), 0);
		val = hf_send(2, 0xffff, buf, sizeof(buf), 0);
		if (val)
			printf("hf_send(): error %d\n", val);
	}
}

void app_main(void)
{
	if (hf_cpuid() == 0){
		hf_spawn(thread, 0, 0, 0, "sender", 4096);
	}else{
		pktdrv_callback = callback;
	}
}
