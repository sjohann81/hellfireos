APP_DIR = $(SRC_DIR)/$(APP)

app: kernel
	$(CC) $(CFLAGS) \
		$(APP_DIR)/xtea.c \
		$(APP_DIR)/sec_sendrecv.c \
		$(APP_DIR)/sec_queue.c 
