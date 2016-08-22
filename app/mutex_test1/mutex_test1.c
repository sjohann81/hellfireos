#include <hellfire.h>

mutex_t m;

int32_t shared;

void task(void){
	int32_t itr = 0;

	while(1){
		hf_mtxlock(&m);
		shared = random();
		printf("\nTask %d on critical region.. shared: %d itr: %d", hf_selfid(), shared, itr++);
		hf_mtxunlock(&m);
		delay_ms(1);			// do not hog the CPU!
	}
}


void app_main(void){
	hf_mtxinit(&m);

	hf_spawn(task, 5, 1, 6, "task a", 1024);
	hf_spawn(task, 6, 1, 6, "task b", 1024);
	hf_spawn(task, 7, 1, 7, "task c", 1024);
	hf_spawn(task, 8, 1, 8, "task d", 1024);
	hf_spawn(task, 9, 1, 9, "task e", 1024);
	hf_spawn(task, 10, 1, 10, "task f", 1024);
}
