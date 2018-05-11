#include <hellfire.h>

void thread(void)
{
	for(;;){
		printf("t%d\n", hf_selfid());
		delay_ms(1000);
	}
}

void app_main(void){
	int32_t i;
	
	for (i = 0; i < 20; i++)
		hf_spawn(thread, 0, 0, 0, "thread", 1024);
}

