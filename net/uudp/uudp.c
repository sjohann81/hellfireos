#include <hellfire.h>
#include <ustack.h>
#include <uudp.h>

static struct list *comm_list;
mutex_t uudplock;

/*
this is called from the UDP layer. this all happens inside the network service (ustack_service), as it
calls the ip stack upstream on the reception of data. find the correct packet queue based on port, take
a free buffer from the pool, copy data from the input packet and add the buffer to the correct packet queue.
if no port is configured for reception or there are no more free buffers, data is lost.
*/
static void udp_callback(uint8_t *packet){
	int32_t i, k;
	uint16_t port, len;
	struct uudp *comm_node;
	void *buff;
	
	port = (packet[UDP_HDR_DESTPORT1] << 8) | (packet[UDP_HDR_DESTPORT2] & 0xff);
	k = hf_list_count(comm_list);
	for (i = 0; i < k; i++){
		comm_node = (struct uudp *)hf_list_get(comm_list, i);
		if (comm_node->listen_port == port)
			break;
	}
	
	if (i < k){
		buff = hf_queue_remhead(comm_node->free_buffers);
		if (buff){
			len = (packet[IP_HDR_LEN1] << 8) | (packet[IP_HDR_LEN2] & 0xff);
			memcpy(buff, packet, len);
			hf_queue_addtail(comm_node->pkt_queue, buff);
		}
	}
	
}

int32_t hf_uudp_create(struct uudp *comm, uint16_t listen_port, uint32_t qsize)
{
	int32_t i, k;
	void *ptr;
	struct uudp *comm_node;
	
	if (udp_get_callback() == NULL){
		comm_list = hf_list_init();
		if (!comm_list)
			return ERR_OUT_OF_MEMORY;
		udp_set_callback(udp_callback);
		hf_mtxinit(&uudplock);
	}

	if (listen_port == 0)
		comm->listen_port = (uint16_t)(((uint32_t)random() % 16383) + 49152);
	else
		comm->listen_port = listen_port;
		
	k = hf_list_count(comm_list);
	for (i = 0; i < k; i++){
		comm_node = (struct uudp *)hf_list_get(comm_list, i);
		if (comm_node->listen_port == comm->listen_port)
			return ERR_ERROR;
	}
	
	if (hf_list_append(comm_list, comm))
		return ERR_OUT_OF_MEMORY;

	comm->free_buffers = hf_queue_create(qsize);
	if (!comm->free_buffers)
		return ERR_OUT_OF_MEMORY;
		
	comm->pkt_queue = hf_queue_create(qsize);
	if (!comm->pkt_queue)
		return ERR_OUT_OF_MEMORY;

	for (i = 0; i < qsize; i++){
		ptr = hf_malloc(PACKET_SIZE);
		if (ptr == NULL){
			while (hf_queue_count(comm->free_buffers))
				hf_free(hf_queue_remhead(comm->free_buffers));
			hf_queue_destroy(comm->free_buffers);
			
			while (hf_queue_count(comm->pkt_queue))
				hf_free(hf_queue_remhead(comm->pkt_queue));
			hf_queue_destroy(comm->pkt_queue);
			
			k = hf_list_count(comm_list);
			hf_list_remove(comm_list, k - 1);
			
			return ERR_OUT_OF_MEMORY;
		}
		hf_queue_addtail(comm->free_buffers, ptr);
	}

	return ERR_OK;
}

int32_t hf_uudp_destroy(struct uudp *comm)
{
	int32_t i, k;
	struct uudp *comm_node = NULL;
	
	if (!comm_list)
		return ERR_ERROR;
		
	k = hf_list_count(comm_list);
	for (i = 0; i < k; i++){
		comm_node = (struct uudp *)hf_list_get(comm_list, i);
		if (comm_node->listen_port == comm->listen_port)
			break;
	}
	
	if (!comm_node)
		return ERR_ERROR;
		
	if (comm_node->listen_port != comm->listen_port)
		return ERR_ERROR;
		
	while (hf_queue_count(comm->free_buffers))
		hf_free(hf_queue_remhead(comm->free_buffers));
	hf_queue_destroy(comm->free_buffers);
			
	while (hf_queue_count(comm->pkt_queue))
		hf_free(hf_queue_remhead(comm->pkt_queue));
	hf_queue_destroy(comm->pkt_queue);
			
	hf_list_remove(comm_list, i);
	
	return ERR_OK;
}

/*
UDP receive

this is called from the application layer. since the network service (ustack_service) task is the only
task that calls netif_recv() and accesses the low level input buffer (frame_in), we can do this without locks.
take a UDP datagram reference from the packet queue, copy data to an output buffer and free the reference
(put it back on the pool of free reception buffers).
*/
int32_t hf_uudp_recv(struct uudp *comm, uint8_t src_ip[4], uint16_t *src_port, uint8_t *buf)
{
	uint16_t len;
	uint8_t *ptr;
	
	if (hf_queue_count(comm->pkt_queue) == 0) return 0;
	
	ptr = (uint8_t *)hf_queue_remhead(comm->pkt_queue);
	len = (ptr[UDP_HDR_LEN1] << 8) | (ptr[UDP_HDR_LEN2] & 0xff);
	
	if (len > PACKET_SIZE - ETH_HEADER_SIZE - UDP_HEADER_SIZE)
		return ERR_ERROR;
	
	memcpy(src_ip, &ptr[IP_HDR_SRCADDR1], 4);
	*src_port = (ptr[UDP_HDR_SRCPORT1] << 8) | (ptr[UDP_HDR_SRCPORT2] & 0xff);
	memcpy(buf, &ptr[UDP_DATA_OFS], len - UDP_HEADER_SIZE);
	hf_queue_addtail(comm->free_buffers, ptr);
	
	return len - UDP_HEADER_SIZE;
}

/*
UDP send

this is called from the application layer. any task can be trying to send data (accessing the network
stack downstream), so we need to synchronize the access to the low level output buffer (frame_out).
copy data to the low level output buffer and try to send it through the network.
*/
int32_t hf_uudp_send(struct uudp *comm, uint8_t dst_ip[4], uint16_t dst_port, uint8_t *buf, uint16_t len)
{
	int32_t val, tries = 0;

	if (len > PACKET_SIZE - ETH_HEADER_SIZE - UDP_HEADER_SIZE)
		return ERR_ERROR;
	
	hf_mtxlock(&uudplock);
	memcpy(frame_out + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE, buf, len);
	do {
		val = udp_out(dst_ip, comm->listen_port, dst_port, frame_out + ETH_HEADER_SIZE, len + UDP_HEADER_SIZE);
		if (val == 0) delay_ms(UUDP_DELAY_ON_RETRY);
	} while (val == 0 && tries++ < UUDP_RETRIES);
	hf_mtxunlock(&uudplock);

	return val;
}
