#include <hellfire.h>

void task(void){
	for(;;){
	}
}

void app_main(void){
	int32_t i;
	volatile int32_t time;
	
	time = _readcounter();
	for (i = 0; i < 10; i++)
		hf_spawn(task, 0, 0, 0, "task", 1024);
	time =  _readcounter() - time;
	printf("\ntask creation: %d", time);
	time = _readcounter();
	for (i = 1; i < 11; i++)
		hf_kill(i);
	time = _readcounter() - time;
	printf("\ntask destruction: %d", time);
}
