#include <hellfire.h>
#include <noc.h>

void sender(void)
{
	int32_t i, msg = 0;
	int8_t buf[500];
	
	if (hf_comm_create(hf_selfid(), 0))
		panic(0xff);
	
	while (1){
		for (i = 0; i < hf_ncores(); i++, msg++){
			if (hf_cpuid() != i){
				sprintf(buf, "hello from cpu %d thread %d msg %d\n", hf_cpuid(), hf_selfid(), msg);
				hf_send(i, 1, buf, sizeof(buf), 0);
			}
		}
		delay_ms(10);
	}
}

void receiver(void)
{
	int32_t i;
	int8_t buf[500];
	uint16_t cpu, task, size, val;
	
	if (hf_comm_create(hf_selfid(), 0))
		panic(0xff);
	
	while (1){
		val = hf_recv(&cpu, &task, buf, &size, 0);
		if (val)
			printf("error %d\n", val);
		else
			printf("%s", buf);
	}
}

void sender2(void)
{
	int32_t i, msg = 0;
	int8_t buf[500];
	
	if (hf_comm_create(hf_selfid(), 0))
		panic(0xff);
	
	while (1){
		for (i = 0; i < hf_ncores(); i++, msg++){
			if (hf_cpuid() != i){
				sprintf(buf, "hey from cpu %d thread %d msg %d\n", hf_cpuid(), hf_selfid(), msg);
				hf_send(i, 2, buf, sizeof(buf), 0);
			}
		}
		delay_ms(10);
	}
}

void receiver2(void)
{
	int32_t i;
	int8_t buf[500];
	uint16_t cpu, task, size, val;
	
	if (hf_comm_create(hf_selfid(), 0))
		panic(0xff);
	
	while (1){
		val = hf_recv(&cpu, &task, buf, &size, 0);
		if (val)
			printf("error %d\n", val);
		else
			printf("%s", buf);
	}
}

void app_main(void)
{
	if (hf_cpuid() == 0){
		hf_spawn(sender, 0, 0, 0, "sender", 2048);
	}else{
		hf_spawn(receiver, 0, 0, 0, "receiver", 2048);
	}
	if (hf_cpuid() == 5){
		hf_spawn(sender2, 0, 0, 0, "sender2", 2048);
	}else{
		hf_spawn(receiver2, 0, 0, 0, "receiver2", 2048);
	}
}
