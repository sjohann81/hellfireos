libc:
	$(CC) $(CFLAGS) \
		$(SRC_DIR)/lib/libc/libc.c \
		$(SRC_DIR)/lib/libc/math.c \
		$(SRC_DIR)/lib/misc/crc.c
