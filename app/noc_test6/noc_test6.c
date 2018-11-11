#include <hellfire.h>
#include <noc.h>
#include <noc_rpc.h>

int32_t func(int8_t *in, int8_t *out)
{
	printf("FOI!\n");
	sprintf(out, "haaaaaaammm!!\n");
	
	return 0;
}

void thread(void)
{
	int32_t test = 0;
	int8_t buf[1000];
	int16_t val, i;
	uint32_t cycles;

	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);

	delay_ms(50);

	while (1){
		sprintf(buf, "test %d", test++);
		/* sending data to callback port. message size must small (1 packet) */
//		val = hf_send(1, 0xffff, buf, sizeof(buf), 0);
//		val = hf_send(2, 0xffff, buf, sizeof(buf), 0);
//		if (val)
//			printf("hf_send(): error %d\n", val);
		for (i = 0; i < 1000; i++) {
			cycles = _readcounter();
			hf_call(2, 0, 0, buf, 900, buf, 900);
			cycles = _readcounter() - cycles;
			printf("%s %d\n", buf, cycles);
		}
		
		while(1);
	}
}

void app_main(void)
{
	if (hf_cpuid() == 0){
		hf_spawn(thread, 0, 0, 0, "sender", 4096);
	}else{
		hf_register(0, 0, func, 900, 900);
	}
}
