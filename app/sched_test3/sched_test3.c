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
	hf_spawn(task, 4, 1, 4, "task a", 2048);
	hf_spawn(task, 5, 2, 5, "task b", 2048);
	hf_spawn(task, 7, 2, 7, "task c", 2048);
}
