APP_DIR = $(SRC_DIR)/$(APP)

app: kernel
	$(CC) $(CFLAGS) \
		$(APP_DIR)/dwt97.c 
