#include <hellfire.h>
#include <ustack.h>
#include <uudp.h>

/* send / receive task
 * 
 * i will send a packet out each second.
 * my source port is 30000.
 * 
 * try me:
 * echo -e "foo\x00" | nc -u -w1 192.168.5.10 30000
 * i will catch traffic on port 30000
 */
void sendrecv(void)
{
	struct uudp uudp_comm;
	uint8_t src_addr[4];
	uint16_t port, len;
	uint8_t *buf;
	uint8_t dst_addr[4] = {192, 168, 5, 2};
	uint8_t message[] = "hello world!";
	uint32_t time;
	int32_t err;
	
	err = hf_uudp_create(&uudp_comm, 30000, 8);
	if (err){
		printf("\nerror creating comm");
		for (;;);
	}
	
	buf = hf_malloc(PACKET_SIZE);
	if (!buf){
		printf("\nerror creating buffer");
		for (;;);
	}
	
	time = _readcounter();
	
	while(1){
		if ((_readcounter() - time) > CPU_SPEED){
			printf("\nsending packet..");
			hf_uudp_send(&uudp_comm, dst_addr, 8855, message, sizeof(message));
			time = _readcounter();
		}
		len = hf_uudp_recv(&uudp_comm, src_addr, &port, buf);
		if (len){
			printf("\n(%d) from %d.%d.%d.%d, port %d: %s", len, src_addr[0], src_addr[1], src_addr[2], src_addr[3], port, buf);
		}
	}
}

void app_main(void)
{
	hf_spawn(sendrecv, 0, 0, 0, "sendrecv", 2048);
}
