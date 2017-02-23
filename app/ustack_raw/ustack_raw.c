#include <hellfire.h>
#include <ustack.h>

void udp_callback(uint8_t *packet){
	printf("\nhi! %s datalen: %d", &packet[UDP_DATA_OFS], packet[UDP_HDR_LEN1] << 8 | packet[UDP_HDR_LEN2]);
}

void task(void){
	uint8_t addr[4] = {192, 168, 5, 2};
	uint16_t bytes;
	
	udp_set_callback(udp_callback);
	
	delay_ms(4000);
	while(1){
		printf("\nsending packet..");
		memset(frame_out + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE, 0xaa, 200 + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE);
		bytes = udp_out(addr, 666, 777, frame_out + ETH_HEADER_SIZE, 200 + UDP_HEADER_SIZE);
		if (!bytes) printf("nothing sent. maybe an ARP request was needed?");
		delay_ms(2000);
	}
}

void app_main(void){
	hf_spawn(task, 0, 0, 0, "task", 4096);
}
