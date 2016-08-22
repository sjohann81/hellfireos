#include <hellfire.h>

#define N 13

sem_t empty, full;
sem_t m;

int32_t in=0, out=0, buffer[N];

void producer(void){
	int32_t item;

	while(1){
		item=random();
		hf_semwait(&empty);
		hf_semwait(&m);
			buffer[in]=item;
			printf("\nproducer %d putting at %d", hf_selfid(), in);
			in=(in+1)%N;
		hf_sempost(&m);
		hf_sempost(&full);
	}
}

void consumer(void){
	int32_t item;

	while(1){
		hf_semwait(&full);
		hf_semwait(&m);
			item=buffer[out];
			printf("\nconsumer %d getting from %d", hf_selfid(), out);
			out=(out+1)%N;
		hf_sempost(&m);
		hf_sempost(&empty);
		item=0;
	}
}

void app_main(void){
	hf_seminit(&empty,N);
	hf_seminit(&full,0);
	hf_seminit(&m,1);

	hf_spawn(producer, 0, 0, 0, "Producer A", 1024);
	hf_spawn(producer, 0, 0, 0, "Producer B", 1024);
	hf_spawn(producer, 0, 0, 0, "Producer C", 1024);
	hf_spawn(consumer, 0, 0, 0, "Consumer A", 1024);
	hf_spawn(consumer, 0, 0, 0, "Consumer B", 1024);
	hf_spawn(consumer, 0, 0, 0, "Consumer C", 1024);
}
