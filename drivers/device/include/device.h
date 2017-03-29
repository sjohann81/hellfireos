#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2

struct device {
	int32_t (*dev_open)(uint32_t flags);
	int32_t (*dev_read)(void *buf, uint32_t size);		/* size is ignored for block devices */
	int32_t (*dev_write)(void *buf, uint32_t size);
	int32_t (*dev_close)(void);
	int32_t (*dev_ioctl)(uint32_t request, void *pval);
	void *ptr;						/* pointer to device specific data
								(e.g struct fs_blkdevice, struct blk_device
								or struct chr_device) */
};

int32_t hf_dev_open(struct device *dev, uint32_t flags);
int32_t hf_dev_read(struct device *dev, void *buf, uint32_t size);
int32_t hf_dev_write(struct device *dev, void *buf, uint32_t size);
int32_t hf_dev_close(struct device *dev);
int32_t hf_dev_ioctl(struct device *dev, uint32_t request, void *pval);
