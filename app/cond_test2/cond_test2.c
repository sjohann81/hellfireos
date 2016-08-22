#include <hellfire.h>

#define N 13

cond_t cond;
mutex_t m;

void task(void){
	for (;;){
		printf("\nhello from task %d, waiting condition...", hf_selfid());
		hf_condwait(&cond, &m);
		printf("\ntask %d, received signal...", hf_selfid());
	}
}

void task2(void){
	for (;;){
		delay_ms(50);
		printf("\nsignalling task... just one task");
		hf_condsignal(&cond);
		delay_ms(50);
		printf("\nsignalling task... everybody");
		hf_condbroadcast(&cond);
	}
}


void app_main(void){
	int32_t i;
	
	hf_condinit(&cond);
	hf_mtxinit(&m);

	for (i = 0; i < 6; i++)
		hf_spawn(task, 0, 0, 0, "task", 1024);
	
	hf_spawn(task2, 0, 0, 0, "task2", 1024);
}
