/* file:          udp.c
 * description:   UDP protocol implementation (RFC768) and basic services
 *                test: echo "foo" | nc -u -w1 192.168.5.10 7
 * date:          12/2013
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <hellfire.h>
#include <ustack.h>

static void (*udp_callback)(uint8_t *packet);

static uint16_t udpchksum(uint8_t *packet, uint16_t len)
{
	uint32_t sum = 0;
	uint16_t i;

	sum += ((uint32_t)packet[IP_HDR_SRCADDR1] << 8) | (uint32_t)packet[IP_HDR_SRCADDR2];
	sum += ((uint32_t)packet[IP_HDR_SRCADDR3] << 8) | (uint32_t)packet[IP_HDR_SRCADDR4];
	sum += ((uint32_t)packet[IP_HDR_DESTADDR1] << 8) | (uint32_t)packet[IP_HDR_DESTADDR2];
	sum += ((uint32_t)packet[IP_HDR_DESTADDR3] << 8) |(uint32_t)packet[IP_HDR_DESTADDR4];
	sum += IP_PROTO_UDP;
	sum += len;
	for(i = IP_HEADER_SIZE; i < len + IP_HEADER_SIZE-1; i += 2)
		sum += ((uint32_t)packet[i] << 8) | (uint32_t)packet[i + 1];
	if (len & 1)
		sum += (uint32_t)packet[len + IP_HEADER_SIZE-1] << 8;
    	while (sum & 0xffff0000)
		sum = (sum & 0xffff) + (sum >> 16);
	sum = ~sum & 0xffff;

	return (uint16_t)sum;
}

int32_t udp_out(uint8_t dst_addr[4], uint16_t src_port, uint16_t dst_port, uint8_t *packet, uint16_t len)
{
	uint16_t chksum;
	int32_t val;
	
	packet[IP_HDR_SRCADDR1] = myip[0];
	packet[IP_HDR_SRCADDR2] = myip[1];
	packet[IP_HDR_SRCADDR3] = myip[2];
	packet[IP_HDR_SRCADDR4] = myip[3];
	packet[IP_HDR_DESTADDR1] = dst_addr[0];
	packet[IP_HDR_DESTADDR2] = dst_addr[1];
	packet[IP_HDR_DESTADDR3] = dst_addr[2];
	packet[IP_HDR_DESTADDR4] = dst_addr[3];
	
	packet[UDP_HDR_SRCPORT1] = src_port >> 8;
	packet[UDP_HDR_SRCPORT2] = src_port & 0xff;
	packet[UDP_HDR_DESTPORT1] = dst_port >> 8;
	packet[UDP_HDR_DESTPORT2] = dst_port & 0xff;
	packet[UDP_HDR_LEN1] = len >> 8;
	packet[UDP_HDR_LEN2] = len & 0xff;
	packet[UDP_HDR_CHKSUM1] = 0;
	packet[UDP_HDR_CHKSUM2] = 0;
	packet[IP_HDR_PROTO] = IP_PROTO_UDP;

	chksum = udpchksum(packet, len);
	packet[UDP_HDR_CHKSUM1] = chksum >> 8;
	packet[UDP_HDR_CHKSUM2] = chksum & 0xff;

	val = ip_out(dst_addr, packet, len + IP_HEADER_SIZE);

	return val;
}

int32_t udp_in(uint8_t *packet)
{
	uint8_t dst_addr[4];
	uint16_t src_port, dst_port, datalen, chksum;

	chksum = (packet[UDP_HDR_CHKSUM1] << 8) | packet[UDP_HDR_CHKSUM2];
	datalen = (packet[UDP_HDR_LEN1] << 8) | packet[UDP_HDR_LEN2];

	if (chksum){
		packet[UDP_HDR_CHKSUM1] = 0;
		packet[UDP_HDR_CHKSUM2] = 0;
		if (chksum != udpchksum(packet, datalen)){
			return -1;
		}
	}
	
	src_port = (packet[UDP_HDR_SRCPORT1] << 8) | packet[UDP_HDR_SRCPORT2];
	dst_port = (packet[UDP_HDR_DESTPORT1] << 8) | packet[UDP_HDR_DESTPORT2];

	switch(dst_port){
		case PORT_ECHO:							/* Echo protocol, RFC862 */
			dst_addr[0] = packet[IP_HDR_SRCADDR1];
			dst_addr[1] = packet[IP_HDR_SRCADDR2];
			dst_addr[2] = packet[IP_HDR_SRCADDR3];
			dst_addr[3] = packet[IP_HDR_SRCADDR4];
			udp_out(dst_addr, dst_port, src_port, packet, datalen);
			break;
		case PORT_DISCARD:						/* Discard protocol, RFC863 */
			break;
		default:
			if (udp_callback)
				udp_callback(packet);

			return datalen;
	}

	return 0;
}

void udp_set_callback(void (*callback)(uint8_t *packet))
{
	udp_callback = callback;
}

void *udp_get_callback(void)
{
	return (void *)udp_callback;
}
