#include <hellfire.h>

void task(void){
	int32_t jobs;
	
	for(;;){
		jobs = krnl_task->jobs;
		printf("\n%s (%d)[%d][%d]", krnl_task->name, krnl_task->id, krnl_task->jobs, krnl_task->deadline_misses);
		while (jobs == krnl_task->jobs);
	}
}

void task2(void){
	int32_t jobs;
	
	for(;;){
		printf("\nblocking task 2");
		jobs = krnl_task->jobs;
		hf_block(2);
		while (jobs == krnl_task->jobs);
		delay_ms(100);
		printf("\nunblocking task 2");
		jobs = krnl_task->jobs;
		hf_resume(2);
		while (jobs == krnl_task->jobs);
		delay_ms(100);
		printf("\nkilling task 2");
		jobs = krnl_task->jobs;
		hf_kill(2);
		while (jobs == krnl_task->jobs);
		delay_ms(100);
		printf("\nspawning task 2");
		jobs = krnl_task->jobs;
		hf_spawn(task, 8, 2, 8, "task b", 2048);
		hf_delay(2, 1000);
		while (jobs == krnl_task->jobs);
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
