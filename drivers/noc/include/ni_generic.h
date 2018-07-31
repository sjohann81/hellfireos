int32_t ni_ready(void);
int32_t ni_flush(uint16_t pkt_size);
int32_t ni_read_packet(uint16_t *buf, uint16_t pkt_size);
int32_t ni_write_packet(uint16_t *buf, uint16_t pkt_size);
