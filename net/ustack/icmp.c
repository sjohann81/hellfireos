/* file:          icmp.c
 * description:   ICMP echo reply protocol implementation (RFC792)
 * date:          02/2013
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <hellfire.h>
#include <ustack.h>

int32_t icmp_echo_reply(uint8_t *packet, uint16_t len)
{
	uint8_t dst_addr[4];
	uint16_t chksum;

	if (packet[ICMP_HDR_TYPE] == ICMP_ECHO){
		packet[ICMP_HDR_TYPE] = ICMP_ECHO_REPLY;
		chksum = (packet[ICMP_HDR_CHKSUM1] << 8) + packet[ICMP_HDR_CHKSUM2];
		if (chksum > (0xffff - (ICMP_ECHO << 8))){
			chksum += (ICMP_ECHO << 8) + 1;
		}else{
			chksum += (ICMP_ECHO << 8);
		}
		packet[ICMP_HDR_CHKSUM1] = chksum >> 8;
		packet[ICMP_HDR_CHKSUM2] = chksum & 0xff;
		
		dst_addr[0] = packet[IP_HDR_SRCADDR1];
		dst_addr[1] = packet[IP_HDR_SRCADDR2];
		dst_addr[2] = packet[IP_HDR_SRCADDR3];
		dst_addr[3] = packet[IP_HDR_SRCADDR4];
		ip_out(dst_addr, packet, len);
		
		return 0;
	}else{
		// ICMP protocol error (other protocol / not implemented)
		return -1;
	}
}
