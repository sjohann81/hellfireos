#include <hellfire.h>

#define Q_SIZE	10

struct queue *q;
mutex_t m;
	
void sender(void)
{
	int32_t i = 0;
	int8_t *buf;
	
	for(;;){
		if (hf_queue_count(q) < Q_SIZE){
			hf_mtxlock(&m);
			buf = malloc(sizeof(int8_t) * 100);
			if (buf){
				sprintf(buf, "hello from task %d, counting %d", hf_selfid(), i++);
				hf_queue_addtail(q, buf);
			}else{
				printf("malloc() failed!\n");
			}
			hf_mtxunlock(&m);
		}
//		delay_ms(5);
	}
}

void receiver(void)
{
	int8_t *b;
	
	for(;;){
		if (hf_queue_count(q)){
			hf_mtxlock(&m);
			b = hf_queue_remhead(q);
			if (b){
				printf("task %d -> %s\n", hf_selfid(), b);
				free(b);
			}
			hf_mtxunlock(&m);
		}
//		delay_ms(20);
	}
}

void log(void)
{
	for(;;){
		printf("queue: %d\n", hf_queue_count(q));
		hf_yield();
	}
}

void app_main(void){
	hf_mtxinit(&m);
	q = hf_queue_create(Q_SIZE);

	hf_spawn(sender, 0, 0, 0, "sender 1", 1024);
	hf_spawn(sender, 0, 0, 0, "sender 2", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 1", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 2", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 3", 1024);
	hf_spawn(log, 100, 1, 100, "log", 512);
}

