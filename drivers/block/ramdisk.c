#include <hellfire.h>
#include <block.h>
#include <ramdisk.h>

static int8_t *ramarena;
static uint32_t rampos = -1, lastpos = -1;
static struct blk_info ramdisk_info;

int32_t ramdisk_open(uint32_t flags)
{
	return 0;
}

int32_t ramdisk_read(void *buf, uint32_t size)
{
	if ((rampos > lastpos) || size < 1) {
		kprintf("\nread() error: invalid read");
		for(;;);
		
		return -1;
	}
#if RAMDISK_DEBUG == 1
	kprintf("\nDEBUG: read() block %d", rampos);
#endif
	memcpy(buf, ramarena + rampos * ramdisk_info.bytes_sector, ramdisk_info.bytes_sector);
	rampos++;

	return 0;
}

int32_t ramdisk_write(void *buf, uint32_t size)
{
	if ((rampos > lastpos) || size < 1) {
		kprintf("\nwrite() error: invalid write");
		for(;;);
		
		return -1;
	}
#if RAMDISK_DEBUG == 1
	kprintf("\nDEBUG: write() block %d", rampos);
	hexdump(buf, ramdisk_info.bytes_sector);
#endif
	memcpy(ramarena + rampos * ramdisk_info.bytes_sector, buf, ramdisk_info.bytes_sector);
	rampos++;
	
	return 0;
}

int32_t ramdisk_close(void)
{
	return 0;
}

int32_t ramdisk_ioctl(uint32_t request, void *pval)
{
	static struct blk_info *infoptr;
	
	switch (request){
	case DISK_INIT:
		ramdisk_info.num_cylinders = 0;
		ramdisk_info.num_heads = 0;
		ramdisk_info.sectors_track = 0;
		ramdisk_info.num_sectors = (uint32_t)pval;
		ramdisk_info.bytes_sector = 128;
		ramdisk_info.media_desc = 0x1000;

		ramarena = (int8_t *)hf_malloc((size_t)pval * ramdisk_info.bytes_sector);
		if (!ramarena) return -1;		
		rampos = 0;
		lastpos = (int32_t)pval * ramdisk_info.bytes_sector - 1;
		kprintf("\nKERNEL: ramdisk initialized, %d bytes", lastpos + 1);
		break;
	case DISK_GETINFO:
		infoptr = (struct blk_info *)pval;
		*infoptr = ramdisk_info;
		break;
	case DISK_SEEKSET:
		rampos = (uint32_t)pval;
		break;
	case DISK_SEEKCUR:
		return rampos;
	case DISK_SEEKEND:
		rampos = lastpos;
		break;
	case DISK_FINISH:
		hf_free(ramarena);
		rampos = -1;
		lastpos = -1;
		break;
	default:
		return -1;
	}
	
	return 0;
}
