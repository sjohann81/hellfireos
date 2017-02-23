#include <hellfire.h>
#include <ustack.h>
#include <uudp.h>

struct uudp uudp_comm;

/* i will send a packet out each second.
 * my source port is 30000.
 */
void task_sender(void){
	uint8_t dst_addr[4] = {192, 168, 5, 2};
	uint8_t message[300];
	uint32_t pkt = 0;
	uint64_t time;
	
	while(1){
		time = _read_us();
		sprintf((int8_t *)message, "\nsystem uptime: %d hours (%d.%d seconds), sending packet #%d", (uint32_t)(time/1000000) / 3600, (uint32_t)(time/1000000), (uint32_t)((time/1000) - (time/1000000) * 1000), pkt++);
		hf_uudp_send(&uudp_comm, dst_addr, 8855, message, strlen((int8_t *)message));
		delay_ms(1000);
	}
}

/* try me:
 * echo -e "foo\x00" | nc -u -w1 192.168.5.10 30000
 * i will catch traffic on port 30000
 */
void task_receiver(void){
	uint8_t src_addr[4];
	uint16_t port, len;
	uint8_t *buf;
	
	buf = hf_malloc(PACKET_SIZE);
	if (!buf){
		printf("\nerror creating buffer");
		for (;;);
	}
	
	while(1){
		len = hf_uudp_recv(&uudp_comm, src_addr, &port, buf);
		if (len){
			printf("\n(%d) from %d.%d.%d.%d, port %d: %s", len, src_addr[0], src_addr[1], src_addr[2], src_addr[3], port, buf);
		}
	}
}

void app_main(void){
	int32_t err;
	
	err = hf_uudp_create(&uudp_comm, 30000, 8);
	if (err){
		printf("\nerror creating comm");
		for (;;);
	}
	
	hf_spawn(task_sender, 0, 0, 0, "task", 2048);
	hf_spawn(task_receiver, 0, 0, 0, "task", 2048);
}
