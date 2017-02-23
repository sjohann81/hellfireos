spi:
	$(CC) $(CFLAGS) \
		$(SRC_DIR)/drivers/spi/spi.c \
		$(SRC_DIR)/drivers/spi/eeprom25lcxx.c \
		$(SRC_DIR)/drivers/spi/sram23lcxx.c \
		$(SRC_DIR)/drivers/spi/mcp23s17.c \
		$(SRC_DIR)/drivers/spi/enc28j60.c
