#include <hellfire.h>
#include <ustack.h>

uint8_t myip[4] = {0, 0, 0, 0};
uint8_t mynm[4] = {0, 0, 0, 0};
uint8_t mygw[4] = {0, 0, 0, 0};

static int32_t is_broadcast_mac(uint8_t *frame)
{
	uint8_t bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	if (!memcmp(&frame[ETH_DA_OFS], bcast_addr, 6))
		return 1;
	else
		return 0;
}

static int32_t is_any_mac(uint8_t *frame)
{
	uint8_t bcast_addr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	if (!memcmp(&frame[ETH_DA_OFS], bcast_addr, 6))
		return 1;
	else
		return 0;
}

static int32_t is_local_mac(uint8_t *frame)
{
	if (!memcmp(&frame[ETH_DA_OFS], mymac, 6))
		return 1;
	else
		return 0;
}


/*
 * network interface send() / receive()
 * 
 * -abstract the link layer
 * -handle interface specific protocols
 * -handle broadcast frames
 * -handle ARP protocol
 */ 
uint16_t netif_send(uint8_t *packet, uint16_t len)
{
	uint8_t mac[6], ip[4];
	uint8_t *frame;
	int32_t l, arp_r = 0;
	
	frame = packet - ETH_HEADER_SIZE;
	memcpy(ip, &packet[IP_HDR_DESTADDR1], 4);
	
	if (ip_addr_isany(ip) || ip_addr_isbroadcast(ip, mynm)){
		memset(mac, 0xff, 6);
		arp_r = 1;
	}else{
		if (ip_addr_ismulticast(ip)){
		    /* Hash IP multicast address to MAC address. */
		    mac[0] = 0x01; mac[1] = 0x0; mac[2] = 0x5e;
		    mac[3] = ip[1] & 0x7f; mac[4] = ip[2]; mac[5] = ip[3];
		    arp_r = 1;
		}else{
			if (!ip_addr_maskcmp(ip, myip, mynm))
				memcpy(ip, mygw, 4);
				
			arp_r = arp_check(ip, mac);
		}
	}

	if (arp_r){
		memcpy(&frame[ETH_SA_OFS], mymac, 6);
		memcpy(&frame[ETH_DA_OFS], mac, 6);
		frame[ETH_TYPE_OFS] = FRAME_IP >> 8;
		frame[ETH_TYPE_OFS + 1] = FRAME_IP & 0xff;
		en_ll_output(frame, len + ETH_HEADER_SIZE);
			
		return len + ETH_HEADER_SIZE;
	}else{
		l = arp_request(frame);
		memcpy(&frame[ARP_TARGET_IP_OFS], ip, 4);
		en_ll_output(frame, l + ETH_HEADER_SIZE);
	
		return 0;
	}
}

uint16_t netif_recv(uint8_t *packet)
{
	int32_t len = 0, ll_len;
	uint16_t type;
	uint8_t *frame;
	
	frame = packet - ETH_HEADER_SIZE;
	ll_len = en_ll_input(frame);
	
	if (ll_len > 0){
		if (is_local_mac(frame) || is_broadcast_mac(frame) || is_any_mac(frame)){
			type = frame[ETH_TYPE_OFS] << 8 | frame[ETH_TYPE_OFS + 1];
			
			switch(type){
			case FRAME_ARP:
				if ((frame[ARP_HARDW_OFS] << 8 | frame[ARP_HARDW_OFS + 1]) == HARDW_ETH10){
					if ((frame[ARP_PROT_OFS] << 8 | frame[ARP_PROT_OFS + 1]) == FRAME_IP){
						if ((frame[ARP_HLEN_PLEN_OFS] << 8 | frame[ARP_HLEN_PLEN_OFS + 1]) == IP_HLEN_PLEN){
							if ((frame[ARP_OPCODE_OFS] << 8 | frame[ARP_OPCODE_OFS + 1]) == OP_ARP_REQUEST){
								if (!memcmp(&frame[ARP_TARGET_IP_OFS], myip, 4)){
									len = arp_reply(frame);
									en_ll_output(frame, len);
									
									return 0;
								}
							}
							if ((frame[ARP_OPCODE_OFS] << 8 | frame[ARP_OPCODE_OFS + 1]) == OP_ARP_ANSWER){
								arp_update(&frame[ARP_SENDER_IP_OFS], &frame[ARP_SENDER_HA_OFS]);
							}
						}
					}
				}
				break;
			case FRAME_IP:
				len = frame[ETH_DATA_OFS + IP_HDR_LEN1] << 8 | frame[ETH_DATA_OFS + IP_HDR_LEN2];
				break;
			case FRAME_IEEE:
				break;
			case FRAME_IPV6:
				break;
			default:
				break;
			}
		}
	}
	
	if (len < 0)
		return 0;
	else
		return len;
}

/* 
 * network stack service
 * 
 * the job of this service includes:
 * -pass a received packet upstream using netif_recv()
 * -poll the interface for link status each ~500ms
 * 
 * we have some issues here. for this mechanism to work with the rest of the network stack,
 * a whole packet should be handled at once (while the ustack_service thread is running).
 * processing of a packet may take some time for a number of reasons, such as a slow SPI
 * driver for the MAC/PHY layers. if the network system becomes unresponsive when a significant
 * number of threads is running, try to increase the quantum (tick) time. this way we can
 * complete the processing of the packet inside the driver quantum, and the priorization
 * mechanism implemented in the BE scheduler works as expected.
 * 
 * an interrupt driven network driver should mark this thread as critical when a packet
 * arrives.
 */
static void ustack_service(void)
{
	uint16_t len;
	uint32_t time, timeout = 500 * (CPU_SPEED / 2000);

	time = _readcounter();
	while(1){
		len = netif_recv(frame_in + ETH_HEADER_SIZE);
		if (len > 0){
			ip_in(myip, frame_in + ETH_HEADER_SIZE, len);
		}
		if (_readcounter() - time > timeout) {		// check link status each ~500ms
			en_watchdog();
			time = _readcounter();
		}
	}
}

void ustack_init(void)
{
	kprintf("\nKERNEL: initializing ustack...");
	if (!frame_in || !frame_out){
		kprintf("\nKERNEL: low level net driver failed");
		return;
	}
	/* configure ip, mask, gateway ... */
	#ifdef MYIP_1 
	myip[0] = MYIP_1; myip[1] = MYIP_2; myip[2] = MYIP_3; myip[3] = MYIP_4;
	#endif
	#ifdef MYNM_1
	mynm[0] = MYNM_1; mynm[1] = MYNM_2; mynm[2] = MYNM_3; mynm[3] = MYNM_4;
	#endif
	#ifdef MYGW_1
	mygw[0] = MYGW_1; mygw[1] = MYGW_2; mygw[2] = MYGW_3; mygw[3] = MYGW_4;
	#endif
	kprintf("\nKERNEL: IP: %d.%d.%d.%d, netmask: %d.%d.%d.%d, gateway: %d.%d.%d.%d",
		myip[0], myip[1], myip[2], myip[3], mynm[0], mynm[1], mynm[2], mynm[3], mygw[0], mygw[1], mygw[2], mygw[3]);
	
	memset(arp_cache, 0, sizeof(arp_cache));
	udp_set_callback(NULL);
	
	/* add the network service */
	hf_spawn(ustack_service, 0, 0, 0, "ustack", 2048);
}
