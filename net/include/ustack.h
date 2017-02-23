/* general configuration definitions */
#define PACKET_SIZE		1518		/* must be even! between 576 and 1518 (max. for Ethernet) */
#define ARP_CACHE_SIZE		16		/* number of entries on the ARP cache */
#define IP_CFG_PING		113

/* SLIP link definitions */
#define SLIP_END		192
#define SLIP_ESC		219
#define SLIP_ESC_END		220
#define SLIP_ESC_ESC		221

// Ethernet link definitions
#define ETH_DA_OFS		0		// Destination MAC address (48 Bit)
#define ETH_SA_OFS		6		// Source MAC address (48 Bit)
#define ETH_TYPE_OFS		12		// Type field (16 Bit)
#define ETH_DATA_OFS		14		// Frame Data
#define ETH_HEADER_SIZE		14

#define FRAME_ARP		0x0806		// ARP frame
#define FRAME_IP		0x0800		// IPv4
#define FRAME_IEEE		0x8100 		// IEEE 802.1Q
#define FRAME_IPV6		0x86dd		// IPv6

// ARP definitions
#define ARP_HARDW_OFS		14		// Hardware address type
#define ARP_PROT_OFS		16		// Protocol
#define ARP_HLEN_PLEN_OFS	18		// byte length of each hardw. / prot. address
#define ARP_OPCODE_OFS		20		// Opcode
#define ARP_SENDER_HA_OFS	22		// Hardw. address of sender of this packet
#define ARP_SENDER_IP_OFS	28		// IP address of sender
#define ARP_TARGET_HA_OFS	32		// Hardw. address of target of this packet
#define ARP_TARGET_IP_OFS	38		// IP address of target
#define ARP_FRAME_SIZE		28

#define HARDW_ETH10		1		// hardware-type 10Mbps Ethernet
#define IP_HLEN_PLEN		0x0604		// MAC = 6 byte long, IP = 4 byte long
#define OP_ARP_REQUEST		1		// operations for ARP-frames
#define OP_ARP_ANSWER		2

/* IP layer definitions */
#define IP_HDR_VHL		0
#define IP_HDR_TOS		1
#define IP_HDR_LEN1		2
#define IP_HDR_LEN2		3
#define IP_HDR_IPID1		4
#define IP_HDR_IPID2		5
#define IP_HDR_FLAGS1		6
#define IP_HDR_FLAGS2		7
#define IP_HDR_TTL		8
#define IP_HDR_PROTO		9
#define IP_HDR_CHKSUM1		10
#define IP_HDR_CHKSUM2		11
#define IP_HDR_SRCADDR1		12
#define IP_HDR_SRCADDR2		13
#define IP_HDR_SRCADDR3		14
#define IP_HDR_SRCADDR4		15
#define IP_HDR_DESTADDR1	16
#define IP_HDR_DESTADDR2	17
#define IP_HDR_DESTADDR3	18
#define IP_HDR_DESTADDR4	19
#define IP_DATA_OFS		20
#define IP_HEADER_SIZE		20

#define IP_VER_IHL		0x4500
#define IP_TOS_D		0x0010
#define IP_TOS_T		0x0008
#define IP_TOS_R		0x0004

#define IP_FLAG_DONTFRAG	0x4000
#define IP_FLAG_MOREFRAG	0x2000
#define IP_FRAGOFS_MASK		0x1fff

#define IP_DEFAULT_TTL		64

#define IP_PROTO_ICMP		1
#define IP_PROTO_IGMP		2
#define IP_PROTO_TCP		6
#define IP_PROTO_IGRP		9
#define IP_PROTO_UDP		17
#define IP_PROTO_GRE		47
#define IP_PROTO_ESP		50
#define IP_PROTO_AH		51
#define IP_PROTO_SKIP		57
#define IP_PROTO_EIGRP		88
#define IP_PROTO_OSPF		89
#define IP_PROTO_L2TP		115
#define IP_PROTO_SCTF		132

/* ICMP definitions */
#define ICMP_HDR_TYPE		20
#define ICMP_HDR_CODE		21
#define ICMP_HDR_CHKSUM1	22
#define ICMP_HDR_CHKSUM2	23
#define ICMP_HDR_ID1		24
#define ICMP_HDR_ID2		25
#define ICMP_HDR_SEQNO1		26
#define ICMP_HDR_SEQNO2		27
#define ICMP_ECHO_REPLY		0
#define ICMP_ECHO		8
#define ICMP_HDR_SIZE		8

/* UDP definitions */
#define UDP_HDR_SRCPORT1	20
#define UDP_HDR_SRCPORT2	21
#define UDP_HDR_DESTPORT1	22
#define UDP_HDR_DESTPORT2	23
#define UDP_HDR_LEN1		24
#define UDP_HDR_LEN2		25
#define UDP_HDR_CHKSUM1		26
#define UDP_HDR_CHKSUM2		27
#define UDP_DATA_OFS		28
#define UDP_HEADER_SIZE		8

/* standard ports */
#define PORT_ECHO		7
#define PORT_DISCARD		9
#define PORT_DAYTIME		13
#define PORT_QOTD		17
#define PORT_CHARGEN		19

struct arp_entry {
	uint8_t ip[4];
	uint8_t mac[6];
};

extern uint8_t myip[4];
extern uint8_t mynm[4];
extern uint8_t mygw[4];
extern uint8_t mymac[6];
extern struct arp_entry arp_cache[ARP_CACHE_SIZE];

/* layer 1 */
extern uint8_t *frame_in, *frame_out;
extern uint8_t mymac[6];

extern int32_t en_init();
extern int32_t en_watchdog(void);
extern void en_ll_output(uint8_t *frame, uint16_t size);
extern int32_t en_ll_input(uint8_t *frame);

/* layer 2 */
void ustack_init(void);
uint16_t netif_send(uint8_t *packet, uint16_t len);
uint16_t netif_recv(uint8_t *packet);
int32_t arp_reply(uint8_t *frame);
int32_t arp_request(uint8_t *frame);
int32_t arp_update(uint8_t *ip, uint8_t *mac);
int32_t arp_check(uint8_t *ip, uint8_t *mac);

/* layer 3 */
int32_t ip_addr_maskcmp(uint8_t addr1[4], uint8_t addr2[4], uint8_t mask[4]);
int32_t ip_addr_cmp(uint8_t addr1[4], uint8_t addr2[4]);
int32_t ip_addr_isany(uint8_t addr[4]);
int32_t ip_addr_isbroadcast(uint8_t addr[4], uint8_t mask[4]);
int32_t ip_addr_ismulticast(uint8_t addr[4]);
int32_t ip_out(uint8_t dst_addr[4], uint8_t *packet, uint16_t len);
int32_t ip_in(uint8_t dst_addr[4], uint8_t *packet, uint16_t len);
int32_t icmp_echo_reply(uint8_t *packet, uint16_t len);

/* layer 4 */
int32_t udp_out(uint8_t dst_addr[4], uint16_t src_port, uint16_t dst_port, uint8_t *packet, uint16_t len);
int32_t udp_in(uint8_t *packet);
void udp_set_callback(void (*callback)(uint8_t *packet));
void *udp_get_callback(void);
