#define UUDP_RETRIES		5
#define UUDP_DELAY_ON_RETRY	200		/* delay (in ms) */

struct uudp {
	uint16_t listen_port;
	struct queue *free_buffers;
	struct queue *pkt_queue;
};

int32_t hf_uudp_create(struct uudp *comm, uint16_t listen_port, uint32_t qsize);
int32_t hf_uudp_destroy(struct uudp *comm);
int32_t hf_uudp_recv(struct uudp *comm, uint8_t src_ip[4], uint16_t *src_port, uint8_t *buf);
int32_t hf_uudp_send(struct uudp *comm, uint8_t dst_ip[4], uint16_t dst_port, uint8_t *buf, uint16_t len);
