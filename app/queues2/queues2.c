#include <hellfire.h>

#define Q_SIZE	40

struct queue *q;
mutex_t m;
	
void sender(void)
{
	int32_t i = 0;
	int8_t *buf;
	
	for(;;){
		if (hf_queue_count(q) < Q_SIZE){
			buf = malloc(sizeof(int8_t) * 100);
			if (buf){
				sprintf(buf, "hello from task %d, counting %d (item at %08x, %d free)", hf_selfid(), i++, (uint32_t)buf, hf_freemem());
				hf_mtxlock(&m);
				if (hf_queue_addtail(q, buf)){
					printf("queue is full!\n");
					free(buf);
				}
				hf_mtxunlock(&m);
			}else{
				printf("malloc() failed!\n");
			}
		}
	}
}

void receiver(void)
{
	int8_t *b;
	
	for(;;){
		if (hf_queue_count(q)){
			hf_mtxlock(&m);
			b = hf_queue_remhead(q);
			hf_mtxunlock(&m);
			if (b){
				printf("task %d -> %s\n", hf_selfid(), b);
				free(b);
			}
		}
	}
}

void logthread(void)
{
	for(;;){
		printf("queue: %d (tick time %dus)\n", hf_queue_count(q), hf_ticktime());
		hf_yield();
	}
}

void app_main(void){
	hf_mtxinit(&m);
	q = hf_queue_create(Q_SIZE);

	hf_spawn(sender, 0, 0, 0, "sender 1", 1024);
	hf_spawn(sender, 0, 0, 0, "sender 2", 1024);
	hf_spawn(sender, 0, 0, 0, "sender 3", 1024);
	hf_spawn(sender, 0, 0, 0, "sender 4", 1024);
	hf_spawn(sender, 0, 0, 0, "sender 5", 1024);
	hf_spawn(sender, 0, 0, 0, "sender 6", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 1", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 2", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 3", 1024);
	hf_spawn(logthread, 100, 1, 100, "log", 1024);
}

