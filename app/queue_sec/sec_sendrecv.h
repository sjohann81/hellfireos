struct sec_queue {
	mutex_t m;
	struct queue *q;
};

int32_t sec_send(struct sec_queue *sq, uint32_t q_size, uint8_t *msg, const uint32_t key[4], const uint32_t iv[2]);
int32_t sec_recv(struct sec_queue *sq, uint32_t q_size, uint8_t *msg, const uint32_t key[4], const uint32_t iv[2]);
