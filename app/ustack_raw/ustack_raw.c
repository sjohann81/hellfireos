#include <hellfire.h>
#include <ustack.h>

void udp_callback(uint8_t *packet){
	uint8_t dst_addr[4];
	uint16_t src_port, dst_port;
//	int32_t val;

	src_port = (packet[UDP_HDR_SRCPORT1] << 8) | packet[UDP_HDR_SRCPORT2];
	dst_port = (packet[UDP_HDR_DESTPORT1] << 8) | packet[UDP_HDR_DESTPORT2];

	if (dst_port == UDP_DEFAULT_PORT) {
		dst_addr[0] = packet[IP_HDR_SRCADDR1];
		dst_addr[1] = packet[IP_HDR_SRCADDR2];
		dst_addr[2] = packet[IP_HDR_SRCADDR3];
		dst_addr[3] = packet[IP_HDR_SRCADDR4];
		udp_out(dst_addr, dst_port, src_port, packet, 111 + UDP_HEADER_SIZE);
	}
}

void task(void){
	uint16_t bytes;
	
	udp_set_callback(udp_callback);
	
	while(1){
//		printf("\nsending packet..");
//		memset(frame_out + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE, 0xaa, 200 + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE);
//		bytes = udp_out(addr, 666, 777, frame_out + ETH_HEADER_SIZE, 200 + UDP_HEADER_SIZE);
//		if (!bytes) printf("nothing sent. maybe an ARP request was needed?");
//		delay_ms(2000);
	}
}

void app_main(void){
	hf_spawn(task, 0, 0, 0, "task", 4096);
}
