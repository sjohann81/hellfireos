#include <hellfire.h>

void show_queue(struct queue *q)
{
	int32_t i;
	int32_t *p;
	
	printf("\nshowing the queue...");
	for (i = 0; i < hf_queue_count(q); i++){
		p = hf_queue_get(q, i);
		if (p)
			printf("\nnode %d: %d", i, *p);
		else
			printf("\nnode %d: (null)", i);
	}
}

void task(void)
{
	int32_t i, a, b, c, d;
	struct queue *q;
	
	for (;;){
		q = hf_queue_create(20);
		a = 10;
		b = 15;
		c = 2;
		d = 45;
		printf("\nadding 4 elements to the queue");
		if (hf_queue_addtail(q, &a)) printf("FAIL");
		if (hf_queue_addtail(q, &b)) printf("FAIL");
		if (hf_queue_addtail(q, &c)) printf("FAIL");
		if (hf_queue_addtail(q, &d)) printf("FAIL");
		show_queue(q);
		printf("\nremoving the two elements from the head");
		if (!hf_queue_remhead(q)) printf("FAIL");
		if (!hf_queue_remhead(q)) printf("FAIL");
		show_queue(q);
		panic(0);
	}
}

void app_main(void)
{
	hf_spawn(task, 0, 0, 0, "task", 2048);
}

