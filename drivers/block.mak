block:
	$(CC) $(CFLAGS) \
		$(SRC_DIR)/drivers/block/ramdisk.c \
		$(SRC_DIR)/drivers/block/sdcard.c
