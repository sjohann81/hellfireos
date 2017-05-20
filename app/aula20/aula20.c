#include <hellfire.h>
#include <noc.h>

int8_t *gen_rdm_bytestream(size_t size)
{
  int8_t *stream = malloc (size);
  size_t i;

  for(i = 0; i < size; i++)
  {
		stream[i] = random();
  }
  return stream;
}

void t1(void){
	int32_t msg = 0;
	int8_t *buf;
	uint16_t val;
	uint16_t cpu,size;
	
	printf("CPU %d, executando tarefa 1.", hf_cpuid());
	
	if (hf_comm_create(hf_selfid(), 5001, 0))
		panic(0xff);
	
	//sprintf(buf, "i am cpu %d, thread %d: msg %d size: %d\n", hf_cpuid(), hf_selfid(), msg++, sizeof(buf));
	buf = gen_rdm_bytestream (256);
	val = hf_sendack(2, 5000, buf, 256, 0, 500);
	buf = gen_rdm_bytestream (64);
	val = hf_sendack(3, 5000, buf, 64, 0, 500);
	buf = gen_rdm_bytestream (64);
	val = hf_sendack(4, 5000, buf, 64, 0, 500);
	val = hf_sendack(5, 5000, buf, 64, 0, 500);
	val = hf_sendack(1, 5000, buf, 64, 0, 500);


	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);
	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);
	buf = gen_rdm_bytestream (640);
	val = hf_sendack(3, 5000, buf, 640, 0, 500);

}

void t2(void){
	int32_t msg = 0;
	int8_t *buf;//buf[320];
	uint16_t val;
	uint16_t cpu,size;
	
	if (hf_comm_create(hf_selfid(), 5002, 0))
		panic(0xff);
	

	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d, executando tarefa 2.", hf_cpuid());
	printf("tarefa 2, CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);
	buf = gen_rdm_bytestream (64);
	val = hf_sendack(6, 5000, buf, 64, 0, 500);
	buf = gen_rdm_bytestream (320);
	val = hf_sendack(1, 5000, buf, 320, 0, 500);
	val = hf_sendack(2, 5000, buf, 320, 0, 500);

}

void t3(void){
	int8_t *buf;//buf[64];
	uint16_t val;
	uint16_t cpu,size;
	if (hf_comm_create(hf_selfid(), 5003, 0))
		panic(0xff);

	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d, executando tarefa 3.", hf_cpuid());
	printf("tarefa 3, CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);

}

void t4(void){
	int8_t *buf;//[64];
	uint16_t val;
	uint16_t cpu,size;

	if (hf_comm_create(hf_selfid(), 5004, 0))
		panic(0xff);
	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d, executando tarefa 4.", hf_cpuid());
	printf("tarefa 4, CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);

}
void t5(void){
	int8_t *buf;//[64];
	uint16_t val;
	uint16_t cpu,size;
	
	if (hf_comm_create(hf_selfid(), 5005, 0))
		panic(0xff);

	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d, executando tarefa 5.", hf_cpuid());
	printf("tarefa 5, CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);
}

void t6(void){
	int8_t *buf;//[64];
	uint16_t val;
	uint16_t cpu,size;
	
	if (hf_comm_create(hf_selfid(), 5006, 0))
		panic(0xff);

	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d, executando tarefa 6.", hf_cpuid());
	printf("Tarefa 6, CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);
}



void t7(void){
	int8_t *buf;//[1280];
	uint16_t val;
	uint16_t cpu,size;
	
	if (hf_comm_create(hf_selfid(), 5007, 0))
		panic(0xff);
	val = hf_recvack(&cpu, 5000, buf, &size, 0);
	printf("CPU %d, executando tarefa 7.", hf_cpuid());
	printf("Tarefa 7, CPU %d recebeu tarefa com aresta %d\n", hf_cpuid(), size);
}

void t8(void){
	printf("CPU %d, executando tarefa 7.", hf_cpuid());
}

void t9(void){
	printf("CPU %d, executando tarefa 7.", hf_cpuid());
}

void sender1(void)
{
	int32_t msg = 0;
	int8_t buf[1500];
	uint16_t val;
	
	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);
	
	while (1){
		sprintf(buf, "i am cpu %d, thread %d: msg %d size: %d\n", hf_cpuid(), hf_selfid(), msg++, sizeof(buf));
		val = hf_sendack(3, 5000, buf, sizeof(buf), 0, 500);
		if (val)
			printf("hf_sendack(): error %d\n", val);
	}
}

void receiver2(void)
{
	int8_t buf[1500];
	uint16_t cpu, task, size, val;
	
	if (hf_comm_create(hf_selfid(), 5000, 0))
		panic(0xff);
	
	while (1){
		val = hf_recvack(&cpu, &task, buf, &size, 0);
		if (val)
			printf("hf_recvack(): error %d\n", val);
		else
			printf("%s", buf);
	}
}

void app_main(void)
{
	if (hf_cpuid() == 1){
		hf_spawn(t1, 0, 0, 0, "t1", 4096);
		hf_spawn(t7, 0, 0, 0, "t7", 4096);
	}else if (hf_cpuid() == 2){
		hf_spawn(t2, 0, 0, 0, "t2", 4096);
		hf_spawn(t8, 0, 0, 0, "t8", 4096);
	}else if (hf_cpuid() == 3){
		hf_spawn(t3, 0, 0, 0, "t3", 4096);
		hf_spawn(t9, 0, 0, 0, "t9", 4096);
	}else if (hf_cpuid() == 4){
		hf_spawn(t4, 0, 0, 0, "t4", 4096);
	}else if (hf_cpuid() == 5){
		hf_spawn(t5, 0, 0, 0, "t5", 4096);
	}else if (hf_cpuid() == 6){
		hf_spawn(t6, 0, 0, 0, "t6", 4096);
	}
}
