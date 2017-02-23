ustack:
	$(CC) $(CFLAGS) \
		$(SRC_DIR)/net/ustack/arp.c \
		$(SRC_DIR)/net/ustack/eth_netif.c \
		$(SRC_DIR)/net/ustack/ip.c \
		$(SRC_DIR)/net/ustack/icmp.c \
		$(SRC_DIR)/net/ustack/udp.c \
		$(SRC_DIR)/net/uudp/uudp.c
