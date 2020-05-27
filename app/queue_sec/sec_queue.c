#include <hellfire.h>
#include "sec_sendrecv.h"

#define Q_SIZE	40

struct sec_queue sq;
struct sec_queue *sec = &sq;
	
void sender(void)
{
	uint32_t xtea_key[4] = {0xf0e1d2c3, 0xb4a59687, 0x78695a4b, 0x3c2d1e0f};
	uint32_t iv[2] = {0x11223344, 0x55667788};
	uint8_t message[128] = "the quick brown fox jumps over the lazy dog";
	
	while (1) {
		sec_send(sec, Q_SIZE, message, xtea_key, iv);
		delay_ms(1000);
	}
}

void receiver(void)
{
	uint32_t xtea_key[4] = {0xf0e1d2c3, 0xb4a59687, 0x78695a4b, 0x3c2d1e0f};
	uint32_t iv[2] = {0x11223344, 0x55667788};
	uint8_t buf[128];

	while (1) {
		sec_recv(sec, Q_SIZE, buf, xtea_key, iv);
		printf("\nmsg: %s", buf);
	}
}

void sniffer(void)
{
	void *elem;
	
	while (1) {
		if (hf_queue_count(sec->q)) {
			elem = hf_queue_get(sec->q, 0);
			hexdump(elem, 128);
		}
	}
}

void app_main(void)
{
	hf_mtxinit(&sec->m);
	sec->q = hf_queue_create(Q_SIZE);
	
	hf_spawn(sender, 0, 0, 0, "sender 1", 1024);
	hf_spawn(receiver, 0, 0, 0, "receiver 1", 1024);
//	hf_spawn(sniffer, 0, 0, 0, "sniffer", 1024);
//	hf_priorityset(2, 30);
}
