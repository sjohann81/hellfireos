#include <hellfire.h>

void task(void){
	int32_t i, k = 1;
	
	for (i = 0; i < 10; i++){
		printf("\ntask %d, task cpu load %d%% (%d%% idle)", hf_selfid(), hf_cpuload(hf_selfid()), hf_cpuload(0));
		delay_ms(50);
	}
	
	while(1){
		printf("\ntask %d running Coremark, iteration %d, task cpu load %d%% (%d%% idle)\n", hf_selfid(), k++, hf_cpuload(hf_selfid()), hf_cpuload(0));
		coremain();
	}
}

void app_main(void){
	hf_spawn(task, 10, 5, 10, "Coremark", 16384);
}
