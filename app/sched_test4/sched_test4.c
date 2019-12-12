#include <hellfire.h>

void task(void){
	int32_t jobs, id;

	id = hf_selfid();
	for(;;){
		jobs = hf_jobs(id);
		printf("\n%s (%d)[%d][%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
		while (jobs == hf_jobs(id));
	}
}

void app_main(void){
	hf_spawn(task, 20, 3, 7, "task a", 2048);
	hf_spawn(task, 5, 2, 4, "task b", 2048);
	hf_spawn(task, 10, 1, 8, "task c", 2048);
}
