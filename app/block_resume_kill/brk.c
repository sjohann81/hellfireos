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

void task2(void){
	int32_t jobs;
	
	for(;;){
		printf("\nblocking task 2");
		jobs = hf_jobs(hf_selfid());
		hf_block(2);
		while (jobs == hf_jobs(hf_selfid()));
		delay_ms(100);
		printf("\nunblocking task 2");
		jobs = hf_jobs(hf_selfid());
		hf_resume(2);
		while (jobs == hf_jobs(hf_selfid()));
		delay_ms(100);
		printf("\nkilling task 2");
		jobs = hf_jobs(hf_selfid());
		hf_kill(2);
		while (jobs == hf_jobs(hf_selfid()));
		delay_ms(100);
		printf("\nspawning task 2");
		jobs = hf_jobs(hf_selfid());
		hf_spawn(task, 8, 2, 8, "task b", 2048);
		hf_delay(2, 1000);
		while (jobs == hf_jobs(hf_selfid()));
		printf("\nend of life");
		hf_kill(hf_selfid());
	}
}

void app_main(void){
	hf_spawn(task, 4, 1, 4, "task a", 2048);
	hf_spawn(task, 8, 2, 8, "task b", 2048);
	hf_spawn(task, 12, 3, 12, "task c", 2048);
	hf_spawn(task, 10, 1, 10, "task d", 2048);
	hf_spawn(task2, 0, 0, 0, "BE task", 2048);
}
