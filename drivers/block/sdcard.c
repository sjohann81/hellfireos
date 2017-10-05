#include <hellfire.h>
#include <spi.h>
#include <block.h>
#include <sdcard.h>

static uint8_t sd_command(uint8_t cmd, uint32_t arg)
{
	uint8_t res;

#if SDCARD_SDHC == 0
	switch (cmd) {
	case READ_SINGLE_BLOCK:
	case READ_MULTIPLE_BLOCKS:
	case WRITE_SINGLE_BLOCK:
	case WRITE_MULTIPLE_BLOCKS:
	case ERASE_BLOCK_START_ADDR:
	case ERASE_BLOCK_END_ADDR:
		arg = arg << 9;
		break;
	default:
		break;
	}
#endif
	spi_sendrecv(0xff);

	spi_sendrecv(cmd | 0x40);
	spi_sendrecv(arg >> 24);
	spi_sendrecv(arg >> 16);
	spi_sendrecv(arg >> 8);
	spi_sendrecv(arg);

	if (cmd == SEND_IF_COND)
		spi_sendrecv(0x87);     
	else 
		spi_sendrecv(0x95); 

	while ((res = spi_sendrecv(0xff)) == 0xff);

	return res;
}

static void sd_init(void)
{
	int32_t i;
	
	spi_setup(SPI_CS0, 0);
	
	for (i = 0; i < 10; i++)
		spi_sendrecv(0xff);
	
	spi_start();
	while (sd_command(GO_IDLE_STATE, 0) != 0x01);
	while (sd_command(SEND_IF_COND, 0x000001AA) != 0x01);
	while (sd_command(APP_CMD, 0) && sd_command(SD_SEND_OP_COND, 0x40000000));
	while (sd_command(READ_OCR, 0) != 0x00);
	
	spi_stop();
}

static void sd_getinfo(struct sd_raw_info *info)
{
	
}

static void sd_read_block(uint32_t block, uint8_t *buf)
{
	int32_t i;

	spi_start();
	
	while (sd_command(READ_SINGLE_BLOCK, block) != 0x00);
	while (spi_sendrecv(0xff) != 0xfe);
	
	for (i = 0; i < 512; i++)
  		buf[i] = spi_sendrecv(0xff);
	
	spi_sendrecv(0xff); 
	spi_sendrecv(0xff); 
	
	spi_stop();
}

static uint8_t sd_write_block(uint32_t block, uint8_t *buf)
{
	int32_t i;
	uint8_t res;

	spi_start();
	while (sd_command(WRITE_SINGLE_BLOCK, block) != 0x00);
	
	spi_sendrecv(0xfe);
	for (i = 0; i < 512; i++)    
  		spi_sendrecv(buf[i]);
	spi_sendrecv(0xff);  
	spi_sendrecv(0xff);

	res = spi_sendrecv(0xff);

	if ((res & 0x1f) != 0x05) {		//res = 0bXXX0AAA1 ; AAA='010' - data accepted
  		spi_stop();			//AAA='101' - CRC error
						//AAA='110' - write error
  		return res;
	}

	while (!spi_sendrecv(0xff));

	spi_stop();
	spi_sendrecv(0xff);

	spi_start();
	while (!spi_sendrecv(0xff));
	spi_stop();

	return 0;
}

/*
 * sd card low level to HellfireOS wrapper
 */
/*
static uint32_t seekpos = -1, lastpos = -1;
static struct blk_info sdcard_info;
 
int32_t sdcard_open(uint32_t flags)
{
	return 0;
}

int32_t sdcard_read(void *buf, uint32_t size)
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

int32_t sdcard_write(void *buf, uint32_t size)
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

int32_t sdcard_close(void)
{
	return 0;
}

int32_t sdcard_ioctl(uint32_t request, void *pval)
{
	static struct blk_info *infoptr;
	
	switch (request){
	case DISK_INIT:
		sdcard_info.num_cylinders = 0;
		sdcard_info.num_heads = 0;
		sdcard_info.sectors_track = 0;
		sdcard_info.num_sectors = (uint32_t)pval;
		sdcard_info.bytes_sector = 512;
		sdcard_info.media_desc = 0x3000;
		
		seekpos = 0;
		lastpos = (int32_t)pval * ramdisk_info.bytes_sector - 1;
		kprintf("\nKERNEL: sdcard initialized, %d bytes", lastpos + 1);
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
*/
