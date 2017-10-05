#define RAMDISK_DEBUG	1

int32_t ramdisk_open(uint32_t flags);
int32_t ramdisk_read(void *buf, uint32_t size);
int32_t ramdisk_write(void *buf, uint32_t size);
int32_t ramdisk_close(void);
int32_t ramdisk_ioctl(uint32_t request, void *pval);
