#define SDCARD_DEBUG			1
#define SDCARD_SDHC			0

#define GO_IDLE_STATE			0
#define SEND_OP_COND			1
#define SEND_IF_COND			8
#define SEND_CSD			9
#define STOP_TRANSMISSION		12
#define SEND_STATUS			13
#define SET_BLOCK_LEN			16
#define READ_SINGLE_BLOCK		17
#define READ_MULTIPLE_BLOCKS		18
#define WRITE_SINGLE_BLOCK		24
#define WRITE_MULTIPLE_BLOCKS		25
#define ERASE_BLOCK_START_ADDR		32
#define ERASE_BLOCK_END_ADDR		33
#define ERASE_SELECTED_BLOCKS		38
#define SD_SEND_OP_COND			41   
#define APP_CMD				55
#define READ_OCR			58
#define CRC_ON_OFF			59

int32_t sdcard_open(uint32_t flags);
int32_t sdcard_read(void *buf, uint32_t size);
int32_t sdcard_write(void *buf, uint32_t size);
int32_t sdcard_close(void);
int32_t sdcard_ioctl(uint32_t request, void *pval);
