#include <hellfire.h>

#define N_READERS	3
#define N_WRITERS	1

sem_t mtx_rc, mtx_wc, mtx, w_db, r_db;
int32_t rc = 0, wc = 0, reads = 0, writes = 0;

void reader(void){
	int32_t i;

	i = hf_selfid();
	while(1){
		hf_semwait(&mtx);
		hf_semwait(&r_db);
		hf_semwait(&mtx_rc);
		rc++;
		if (rc == 1) hf_semwait(&w_db);
		hf_sempost(&mtx_rc);
		hf_sempost(&r_db);
		hf_sempost(&mtx);
		reads++;
		printf("(R) thread %d reading the database... (%d readers, %d reads, %d writes)\n", i, rc, reads, writes);
		hf_semwait(&mtx_rc);
		rc--;
		if (rc == 0) hf_sempost(&w_db);
		hf_sempost(&mtx_rc);
		printf("(R) thread %d using data...\n", i);
	}
}

void writer(void){
	int32_t i;

	i = hf_selfid();
	while(1){
		hf_semwait(&mtx_wc);
		wc++;
		if (wc == 1) hf_semwait(&r_db);
		hf_sempost(&mtx_wc);
		printf("(W) thread %d preparing data...\n", i);
		hf_semwait(&w_db);
		writes++;
		printf("(W) thread %d writing to the database... (%d reads, %d writes)\n", i, reads, writes);
		hf_sempost(&w_db);
		hf_semwait(&mtx_wc);
		wc--;
		if (wc == 0) hf_sempost(&r_db);
		hf_sempost(&mtx_wc);
	}
}

void app_main(void){
	int32_t i;

	hf_seminit(&mtx_rc, 1);
	hf_seminit(&mtx_wc, 1);	
	hf_seminit(&mtx, 1);
	hf_seminit(&w_db, 1);
	hf_seminit(&r_db, 1);

	for(i = 0; i < N_READERS; i++)
		hf_spawn(reader, 0, 0, 0, "reader", 2048);

	for(i = 0; i < N_WRITERS; i++)
		hf_spawn(writer, 0, 0, 0, "writer", 2048);

}

