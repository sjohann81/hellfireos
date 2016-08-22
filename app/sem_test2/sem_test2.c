#include <hellfire.h>

sem_t s1, s2, s3, s4;

void task0(void){
	int i;

	for(;;){
		for(i = 1; i <= 200; i++)
			printf("%d ", i);
		hf_sempost(&s1);
		for(;;);
	}
}

void task1(void){
	int i;

	for(;;){
		hf_semwait(&s1);
		for(i = 200; i <= 400; i++)
			printf("%d ", i);
		hf_sempost(&s2);
		for(;;);
	}
}

void task2(void){
	int i;

	for(;;){
		hf_semwait(&s2);
		for(i = 401; i <= 600; i++)
			printf("%d ", i);
		hf_sempost(&s3);
		for(;;);
	}
}

void task3(void){
	int i;

	for(;;){
		hf_semwait(&s3);
		for(i = 601; i <= 800; i++)
			printf("%d ", i);
		hf_sempost(&s4);
		for(;;);
	}
}

void task4(void){
	int i;

	for(;;){
		hf_semwait(&s4);
		for(i = 801; i <= 1000; i++)
			printf("%d ", i);
		for(;;);
	}
}

void app_main(void){
	hf_seminit(&s1, 0);
	hf_seminit(&s2, 0);
	hf_seminit(&s3, 0);
	hf_seminit(&s4, 0);

	hf_spawn(task0, 6, 1, 6, "task a", 1024);
	hf_spawn(task1, 9, 1, 9, "task b", 1024);
	hf_spawn(task2, 5, 1, 5, "task c", 1024);
	hf_spawn(task3, 7, 1, 7, "task d", 1024);
	hf_spawn(task4, 6, 1, 6, "task e", 1024);
}
