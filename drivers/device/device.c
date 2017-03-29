#include <hellfire.h>
#include <device.h>

int32_t hf_dev_open(struct device *dev, uint32_t flags)
{
	int32_t err;
	
	err = dev->dev_open(flags);
	if (err)
		kprintf("\nhf_dev_open: error");
	
	return err;
}

int32_t hf_dev_read(struct device *dev, void *buf, uint32_t size)
{
	int32_t err;
	
	err = dev->dev_read(buf, size);
	if (err)
		kprintf("\nhf_dev_read: error");
	
	return err;
}

int32_t hf_dev_write(struct device *dev, void *buf, uint32_t size)
{
	int32_t err;
	
	err = dev->dev_write(buf, size);
	if (err)
		kprintf("\nhf_dev_write: error");
	
	return err;
}

int32_t hf_dev_close(struct device *dev)
{
	int32_t err;
	
	err = dev->dev_close();
	if (err)
		kprintf("\nhf_dev_close: error");
		
	return err;
}

int32_t hf_dev_ioctl(struct device *dev, uint32_t request, void *pval)
{
	return dev->dev_ioctl(request, pval);
}
