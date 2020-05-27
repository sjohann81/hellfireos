#include <hellfire.h>
#include "xtea.h"
#include "sec_sendrecv.h"

int32_t sec_send(struct sec_queue *sq, uint32_t q_size, uint8_t *msg, const uint32_t key[4], const uint32_t iv[2])
{
	int32_t len;
	uint8_t *buf;
	
	len = strlen((char *)msg);
	
	while (1) {
		if (hf_queue_count(sq->q) < q_size){
			buf = malloc(sizeof(int8_t) * len + 16);
			if (buf){
//				memcpy(buf, msg, len);
				xtea_cbc_encrypt(buf, msg, len, key, iv);
				hf_mtxlock(&sq->m);
				if (hf_queue_addtail(sq->q, buf))
					free(buf);
				hf_mtxunlock(&sq->m);
			}else{
				printf("malloc() failed!\n");
			}
			break;
		}
	}
	
	return 0;
}

int32_t sec_recv(struct sec_queue *sq, uint32_t q_size, uint8_t *msg, const uint32_t key[4], const uint32_t iv[2])
{
	uint8_t *b;
	int32_t len = 0;
	
	while (1) {
		if (hf_queue_count(sq->q)){
			hf_mtxlock(&sq->m);
			b = hf_queue_remhead(sq->q);
			hf_mtxunlock(&sq->m);
			if (b){
				len = strlen((char *)b);
//				memcpy(msg, b, len);
				xtea_cbc_decrypt(msg, b, len, key, iv);
				free(b);
			}
			break;
		}
	}
	
	return len;
}
