/* sudo apt-get install inetutils-inetd
 *
 * edit /etc/inetd.conf e remove the comment on the line:
 * bootps  dgram  udp  wait  root  /usr/sbin/bootpd  bootpd
 *
 * edit /etc/bootptab and set individual entries:
 * client:ip=192.168.1.90:sm=255.255.255.0:gw=192.168.1.1:ha=0123456789AB:
 *
 */

#include <hellfire.h>
#include <ustack.h>

static uint8_t mincookie[] = { 99, 130, 83, 99, 255 };
static uint8_t replycookie[] = { 0x63, 0x82, 0x53, 0x63 };
static uint8_t temp_xid[4];

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

uint16_t bootp_request(uint8_t *frame)
{
	int32_t i;
	uint8_t c;
	struct bootp *pk;

	pk = (struct bootp *)(frame + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE);
	memset(pk, 0, sizeof(struct bootp));

	pk->bp_op = BOOTREQUEST;
	pk->bp_htype = HTYPE_ETHERNET;
	pk->bp_hlen = 6;
	for (i = 0; i < 4; i++){
		c = random() & 0xff;
		pk->bp_xid[i] = c;
		temp_xid[i] = c;
	}

	pk->bp_flags |= htons(BPFLAG_BROADCAST);      /* request broadcast reply */
	memcpy(pk->bp_chaddr, mymac, 6);

	memcpy(pk->bp_vend, mincookie, sizeof(mincookie));

	return ETH_HEADER_SIZE + sizeof(struct bootp);
}

uint16_t bootp_handle_reply(uint8_t *frame)
{
	int32_t i;
	struct bootp *pk;

	pk = (struct bootp *)(frame + ETH_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE);

	if (pk->bp_op != BOOTREPLY)
		return 0;

	if (pk->bp_htype != HTYPE_ETHERNET)
		return 0;

	for (i = 0; i < 4; i++){
		if (pk->bp_xid[i] != temp_xid[i])
			return 0;

		if (pk->bp_vend[i] != replycookie[i])
			return 0;
	}

	/* extract ip address, netmask and gateway */
	memcpy(&myip[0], pk->bp_yiaddr, 4);

	uint8_t *ptr = pk->bp_vend + 4;
	while (*ptr != TAG_END){
		switch (*ptr){
		case TAG_SUBNET_MASK:
			memcpy(&mynm[0], &ptr[2], 4);
			break;
		case TAG_GATEWAY:
			memcpy(&mygw[0], &ptr[2], 4);
			break;
		}

		ptr = ptr + ptr[1] + 2;
	}

	return ETH_HEADER_SIZE + sizeof(struct bootp);
}

int32_t bootp_boot(uint8_t *packet)
{
	uint8_t addr[4] = {255, 255, 255, 255};
	uint8_t *frame;
	int32_t tries = 0, ll_len;
	uint16_t type, dest_port;

	frame = packet - ETH_HEADER_SIZE;

	while (1){
		ll_len = en_ll_input(frame);

		if (ll_len > 0){
			if (is_broadcast_mac(frame) || is_any_mac(frame)){
				type = frame[ETH_TYPE_OFS] << 8 | frame[ETH_TYPE_OFS + 1];

				if (type == FRAME_IP){
					dest_port = (packet[UDP_HDR_DESTPORT1] << 8) | packet[UDP_HDR_DESTPORT2];
					if (packet[IP_HDR_PROTO] == IP_PROTO_UDP && dest_port == IPPORT_BOOTPC) {
						if (bootp_handle_reply(frame)) {
							kprintf("\nKERNEL: bootp_boot(), interface configured by BOOTP");
							kprintf("\nKERNEL: IP: %d.%d.%d.%d, NM: %d.%d.%d.%d, GW: %d.%d.%d.%d",
								myip[0], myip[1], myip[2], myip[3],
								mynm[0], mynm[1], mynm[2], mynm[3],
								mygw[0], mygw[1], mygw[2], mygw[3]
							);
							return 1;
						}
					}
				}
			}
			continue;
		}

		if (tries++ == BOOTP_TRIES) break;

		kprintf("\nKERNEL: bootp_boot(), requesting interface configuration");
		bootp_request(frame);
		udp_out(addr, IPPORT_BOOTPC, IPPORT_BOOTPS, packet, sizeof(struct bootp) + UDP_HEADER_SIZE);

		delay_ms(500);
	}

	return 0;
}
