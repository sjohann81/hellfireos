#include <hellfire.h>
#include <noc.h>

void master(){
	int32_t i = 0;
	uint32_t msg = 0;
	uint32_t aux = 0;
	int8_t buf[1028];
	uint16_t val;
	uint8_t *img;
//	uint16_t totalPixels = width * height;
//	img = (uint8_t *) malloc(totalPixels);	

	if (hf_comm_create(hf_selfid(), 3, 0))
		panic(0xff);

//	delay_ms(100);

	//memcpy(buf, img, 1024);

	while(1){
		printf("\nvou mandar");
		for(i = 1; i <= 8; i++){
			memcpy(buf, ++aux, 4);
//			memcpy(buf+4, getChunkFromImage(0, img), 1024);
			val = hf_sendack(i, i*100,buf, sizeof(buf), 0, 1000);
			if (val) printf("sender, hf_sendack(): error %d\n", val);
		}
		while(1){}
	}
}
	
void slave(){
	
	int8_t buf[1028];
	uint16_t cpu, task, size, val;
	uint8_t data[1024];
	int id;

	if (hf_comm_create(hf_selfid(), hf_cpuid()*100, 0))
		panic(0xff);

//	delay_ms(30);

	while(1){
		printf("\nestou esperando");	
//		val = hf_recvack(&cpu, &task, buf, sizeof(dataChunk), 0);
		val = hf_recvack(&cpu, &task, buf, &size, 0);
			if (val){
				printf("error %d\n", val);
			}
			else{
				memcpy(&id, buf, 4);
				memcpy(data, buf+4, 1024);
				printf("\nDataChunk.id: %i, Datachunk.data: %d", id, data);
				printf("\nMessage Received ! From, CPU: %d Task:%d, EU SOU: %d",cpu, task, hf_cpuid());
			}
			while(1){}
	}
}

void app_main(void)
{
	if (hf_cpuid() == 0){
		hf_spawn(master, 0, 0, 0, "sender", 4096);
	}else{
		hf_spawn(slave, 0, 0, 0, "receiver", 4096);
	}
}
