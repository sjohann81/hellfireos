APP_DIR = $(SRC_DIR)/$(APP)

app: kernel
	$(CC) $(CFLAGS) \
		$(APP_DIR)/noc_test4.c 
