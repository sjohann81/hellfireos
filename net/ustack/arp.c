/* file:          arp.c
 * description:   ARP protocol implementation (RFC826 and RFC5227)
 * date:          08/2016
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <hellfire.h>
#include <ustack.h>

struct arp_entry arp_cache[ARP_CACHE_SIZE];
static uint32_t arp_index = 0;

int32_t arp_reply(uint8_t *frame)
{
	uint8_t mac[6], ip[4];
	
	/* ethernet */
	memcpy(mac, &frame[ETH_SA_OFS], 6);
	memcpy(&frame[ETH_SA_OFS], mymac, 6);
	memcpy(&frame[ETH_DA_OFS], mac, 6);
	frame[ETH_TYPE_OFS] = FRAME_ARP >> 8;
	frame[ETH_TYPE_OFS + 1] = FRAME_ARP & 0xff;
	
	/* ARP */
	memcpy(ip, &frame[ARP_SENDER_IP_OFS], 4);
	frame[ARP_HARDW_OFS] = HARDW_ETH10 >> 8;
	frame[ARP_HARDW_OFS + 1] = HARDW_ETH10 & 0xff;
	frame[ARP_PROT_OFS] = FRAME_IP >> 8;
	frame[ARP_PROT_OFS + 1] = FRAME_IP & 0xff;
	frame[ARP_HLEN_PLEN_OFS] = IP_HLEN_PLEN >> 8;
	frame[ARP_HLEN_PLEN_OFS + 1] = IP_HLEN_PLEN & 0xff;
	frame[ARP_OPCODE_OFS] = OP_ARP_ANSWER >> 8;
	frame[ARP_OPCODE_OFS + 1] = OP_ARP_ANSWER & 0xff;
	memcpy(&frame[ARP_SENDER_HA_OFS], mymac, 6);
	memcpy(&frame[ARP_SENDER_IP_OFS], myip, 4);
	memcpy(&frame[ARP_TARGET_HA_OFS], mac, 6);
	memcpy(&frame[ARP_TARGET_IP_OFS], ip, 4);
	
	return ETH_HEADER_SIZE + ARP_FRAME_SIZE;
}

int32_t arp_request(uint8_t *frame)
{
	/* ethernet */
	memcpy(&frame[ETH_SA_OFS], mymac, 6);
	memset(&frame[ETH_DA_OFS], 0xff, 6);
	frame[ETH_TYPE_OFS] = FRAME_ARP >> 8;
	frame[ETH_TYPE_OFS + 1] = FRAME_ARP & 0xff;
	
	/* ARP */
	frame[ARP_HARDW_OFS] = HARDW_ETH10 >> 8;
	frame[ARP_HARDW_OFS + 1] = HARDW_ETH10 & 0xff;
	frame[ARP_PROT_OFS] = FRAME_IP >> 8;
	frame[ARP_PROT_OFS + 1] = FRAME_IP & 0xff;
	frame[ARP_HLEN_PLEN_OFS] = IP_HLEN_PLEN >> 8;
	frame[ARP_HLEN_PLEN_OFS + 1] = IP_HLEN_PLEN & 0xff;
	frame[ARP_OPCODE_OFS] = OP_ARP_REQUEST >> 8;
	frame[ARP_OPCODE_OFS + 1] = OP_ARP_REQUEST & 0xff;
	memcpy(&frame[ARP_SENDER_HA_OFS], mymac, 6);
	memcpy(&frame[ARP_SENDER_IP_OFS], myip, 4);
	memset(&frame[ARP_TARGET_HA_OFS], 0x00, 6);
	memcpy(&frame[ARP_TARGET_IP_OFS], myip, 4);	/* field needs to be updated if not gratuitous ARP */
	
	return ETH_HEADER_SIZE + ARP_FRAME_SIZE;
}

int32_t arp_update(uint8_t *ip, uint8_t *mac)
{
	int32_t i;
	
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		if (!memcmp(ip, arp_cache[i].ip, 4)) break;
	
	if (i < ARP_CACHE_SIZE){
		memcpy(arp_cache[i].mac, mac, 6);
		
		return 1;
	}else{
		memcpy(arp_cache[arp_index].ip, ip, 4);
		memcpy(arp_cache[arp_index].mac, mac, 6);
		i = arp_index;
		arp_index = (arp_index + 1) % ARP_CACHE_SIZE;
		
		return 0;
	}
}

int32_t arp_check(uint8_t *ip, uint8_t *mac)
{
	int32_t i;
	
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		if (!memcmp(ip, arp_cache[i].ip, 4)) break;
	
	if (i < ARP_CACHE_SIZE){
		memcpy(mac, arp_cache[i].mac, 6);
		
		return 1;
	}else{
		return 0;
	}
}
