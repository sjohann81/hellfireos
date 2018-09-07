/* file:          ip.c
 * description:   IPv4 protocol implementation (RFC791)
 * date:          02/2013
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <hellfire.h>
#include <ustack.h>

static uint32_t ipchksum(uint8_t *packet)
{
	uint32_t sum=0;
	uint16_t i;

	for(i = 0; i < 20; i += 2)
		sum += ((uint32_t)packet[i] << 8) | (uint32_t)packet[i + 1];
	while (sum & 0xffff0000)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum;
}

int32_t ip_addr_maskcmp(uint8_t addr1[4], uint8_t addr2[4], uint8_t mask[4])
{
	int32_t i;
	uint8_t ad1[4], ad2[4];

	for (i = 0; i < 4; i++){
		ad1[i] = addr1[i] & mask[i];
		ad2[i] = addr2[i] & mask[i];
	}

	if (!memcmp(ad1, ad2, 4))
		return 1;
	else
		return 0;
}

int32_t ip_addr_cmp(uint8_t addr1[4], uint8_t addr2[4])
{
	if (!memcmp(addr1, addr2, 4))
		return 1;
	else
		return 0;
}

int32_t ip_addr_isany(uint8_t addr[4])
{
	uint8_t addr_any[4] = {0x00, 0x00, 0x00, 0x00};

	if (!memcmp(addr, addr_any, 4))
		return 1;
	else
		return 0;
}

int32_t ip_addr_isbroadcast(uint8_t addr[4], uint8_t mask[4])
{
	int32_t i;
	uint8_t ad[4], msk[4];
	uint8_t addr_any[4] = {0x00, 0x00, 0x00, 0x00};
	uint8_t addr_all[4] = {0xff, 0xff, 0xff, 0xff};

	for (i = 0; i < 4; i++){
		ad[i] = addr[i] & ~mask[i];
		msk[i] = ~mask[i] & 0xff;
	}

	if (!memcmp(ad, msk, 4) || !memcmp(ad, addr_any, 4) | !memcmp(ad, addr_all, 4))
		return 1;
	else
		return 0;
}

int32_t ip_addr_ismulticast(uint8_t addr[4])
{
	int32_t i;
	uint8_t ad[4];
	uint8_t ad1[4] = {0xf0, 0x00, 0x00, 0x00};
	uint8_t ad2[4] = {0xe0, 0x00, 0x00, 0x00};

	for (i = 0; i < 4; i++) ad[i] = addr[i] & ad1[i];

	if (!memcmp(ad, ad2, 4))
		return 1;
	else
		return 0;
}

int32_t ip_out(uint8_t dst_addr[4], uint8_t *packet, uint16_t len)
{
	uint32_t sum;
	int32_t val;

	packet[IP_HDR_VHL] = IP_VER_IHL >> 8;
	packet[IP_HDR_TOS] = 0x00;
	packet[IP_HDR_LEN1] = len >> 8;
	packet[IP_HDR_LEN2] = len & 0xff;
	packet[IP_HDR_IPID1] = 0x00;
	packet[IP_HDR_IPID2] = 0x00;
	packet[IP_HDR_FLAGS1] = 0x00;
	packet[IP_HDR_FLAGS2] = 0x00;
	packet[IP_HDR_TTL] = IP_DEFAULT_TTL;
	/* IP_HDR_PROTO ?? */
	packet[IP_HDR_SRCADDR1] = myip[0];
	packet[IP_HDR_SRCADDR2] = myip[1];
	packet[IP_HDR_SRCADDR3] = myip[2];
	packet[IP_HDR_SRCADDR4] = myip[3];
	packet[IP_HDR_DESTADDR1] = dst_addr[0];
	packet[IP_HDR_DESTADDR2] = dst_addr[1];
	packet[IP_HDR_DESTADDR3] = dst_addr[2];
	packet[IP_HDR_DESTADDR4] = dst_addr[3];
	packet[IP_HDR_CHKSUM1] = packet[IP_HDR_CHKSUM2] = 0;
	sum = (~ipchksum(packet)) & 0xffff;
	packet[IP_HDR_CHKSUM1] = sum >> 8;
	packet[IP_HDR_CHKSUM2] = sum & 0xff;

	val = netif_send(packet, len);

	return val;
}

int32_t ip_in(uint8_t dst_addr[4], uint8_t *packet, uint16_t len)
{
	uint8_t configured;
	int32_t val = 0;

	configured = myip[0] | myip[1] | myip[2] | myip[3];
	if (packet[IP_HDR_VHL] != (IP_VER_IHL >> 8)) return -1;				/* IP version / options error (not supported) */
	if (((packet[IP_HDR_FLAGS1] << 8) | (packet[IP_HDR_FLAGS2])) &
		(IP_FLAG_MOREFRAG | IP_FRAGOFS_MASK)) return -1;			/* IP fragmented frames not supported */
	if (packet[IP_HDR_TTL] == 0) return -1;						/* TP TTL has expired */
	if (configured){								/* if already configured by a magic ping */
		if (!ip_addr_cmp(&packet[IP_HDR_DESTADDR1], dst_addr)) return -1; 	/* IP destination address error */
		if (ipchksum(packet) != 0xffff) return -1;				/* IP checksum error */

		switch(packet[IP_HDR_PROTO]){
			case IP_PROTO_ICMP:
				val = icmp_echo_reply(packet, len);
				break;
			case IP_PROTO_UDP:
				val = udp_in(packet);
				break;
			default:							/* IP protocol error */
				return -1;
		}
	}else{
		if (packet[IP_HDR_PROTO] == IP_PROTO_ICMP && ~configured &&
			len == IP_CFG_PING + IP_HEADER_SIZE + ICMP_HDR_SIZE){		/* configure the IP address */
			myip[0] = packet[IP_HDR_DESTADDR1];
			myip[1] = packet[IP_HDR_DESTADDR2];
			myip[2] = packet[IP_HDR_DESTADDR3];
			myip[3] = packet[IP_HDR_DESTADDR4];
		}
	}

	return val;
}


