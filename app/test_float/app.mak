APP_DIR = $(SRC_DIR)/$(APP)

app: kernel
	$(CC) $(CFLAGS) \
		$(APP_DIR)/test_float.c 
