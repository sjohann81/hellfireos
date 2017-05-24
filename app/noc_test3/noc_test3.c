#include <hellfire.h>
#include <noc.h>

void sender(void)
{
	int32_t msg = 0;
	int8_t buf[500];
	int16_t val, channel;
	
	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);
		
	delay_ms(50);
	
	// generate a unique channel number for this CPU
	channel = hf_cpuid();
	while (1){
		sprintf(buf, "i am cpu %d, thread %d, channel %d: msg %d size: %d\n", hf_cpuid(), hf_selfid(), channel, msg++, sizeof(buf));
		val = hf_sendack(2, 5000, buf, sizeof(buf), channel, 500);
		if (val)
			printf("hf_sendack(): error %d\n", val);
		delay_ms(50);
	}
}

void receiver(void)
{
	int8_t buf[1500];
	uint16_t cpu, task, size;
	int16_t val;
	int32_t i;
	
	if (hf_comm_create(hf_selfid(), 5000, 0))
		panic(0xff);
	
	while (1){
		i = hf_recvprobe();
		if (i >= 0) {
			val = hf_recvack(&cpu, &task, buf, &size, i);
			if (val)
				printf("hf_recvack(): error %d\n", val);
			else
				printf("%s", buf);
		}
	}
}

void app_main(void)
{
	if (hf_cpuid() == 2){
		hf_spawn(receiver, 0, 0, 0, "receiver", 4096);
	}else{
		hf_spawn(sender, 0, 0, 0, "sender", 4096);
	}
}
