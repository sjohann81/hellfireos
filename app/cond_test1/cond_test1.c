#include <hellfire.h>

#define N 13

cond_t empty, full;
mutex_t m;

int32_t in = 0, out = 0, buffer[N], items = 0;

void producer(void){
	int32_t item;

	while(1){
		item=random();
		hf_mtxlock(&m);
		while (items == N)
			hf_condwait(&empty, &m);
		
		buffer[in]=item;
		items++;
		printf("\nproducer %d putting at %d (%d items)", hf_selfid(), in, items);
		in=(in+1)%N;
		
		hf_condsignal(&full);
		hf_mtxunlock(&m);
	}
}

void consumer(void){
	int32_t item;

	while(1){
		hf_mtxlock(&m);
		while (items == 0)
			hf_condwait(&full, &m);
		
		item=buffer[out];
		items--;
		printf("\nconsumer %d getting from %d (%d items)", hf_selfid(), out, items);
		out=(out+1)%N;
		
		hf_condsignal(&empty);
		hf_mtxunlock(&m);
	}
}

void app_main(void){
	hf_condinit(&empty);
	hf_condinit(&full);
	hf_mtxinit(&m);

	hf_spawn(producer, 0, 0, 0, "Producer A", 1024);
	hf_spawn(producer, 0, 0, 0, "Producer B", 1024);
	hf_spawn(producer, 0, 0, 0, "Producer C", 1024);
	hf_spawn(consumer, 0, 0, 0, "Consumer A", 1024);
	hf_spawn(consumer, 0, 0, 0, "Consumer B", 1024);
	hf_spawn(consumer, 0, 0, 0, "Consumer C", 1024);
}
