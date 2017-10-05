#include <hellfire.h>
#include <noc.h>

void sender(void)
{
	int32_t i;
	uint32_t crc;
	int8_t buf[500];
	int16_t val, channel;
	
	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);
		
	delay_ms(50);
	
	srand(hf_cpuid());
	
	// generate a unique channel number for this CPU
	channel = hf_cpuid();
	while (1){
		for (i = 0; i < sizeof(buf)-4; i++)
			buf[i] = random() % 255;
		crc = hf_crc32(buf, sizeof(buf)-4);
		memcpy(buf+sizeof(buf)-4, &crc, 4);
		val = hf_sendack(2, 5000, buf, sizeof(buf), channel, 500);
		if (val)
			printf("hf_sendack(): error %d\n", val);
		//delay_ms(5);
	}
}

void receiver(void)
{
	int8_t buf[1500];
	uint16_t cpu, port, size;
	int16_t val;
	uint32_t crc;
	int32_t i;
	
	if (hf_comm_create(hf_selfid(), 5000, 0))
		panic(0xff);
	
	while (1){
		i = hf_recvprobe();
		if (i >= 0) {
			val = hf_recvack(&cpu, &port, buf, &size, i);
			if (val){
				printf("hf_recvack(): error %d\n", val);
			} else {
				memcpy(&crc, buf+size-4, 4);
				printf("cpu %d, port %d, channel %d, size %d, crc %08x [free queue: %d]", cpu, port, i, size, crc, hf_queue_count(pktdrv_queue));
				if (hf_crc32(buf, size-4) == crc)
					printf(" (CRC32 pass)\n");
				else
					printf(" (CRC32 fail)\n");
			}
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
