#include <hellfire.h>
#include <device.h>
#include <block.h>
#include <ramdisk.h>
#include <uhfs.h>

void app_main(void){
	struct device ramdisk0 = {ramdisk_open, ramdisk_read, ramdisk_write, ramdisk_close, ramdisk_ioctl, 0};
	struct blk_info ramdisk0info;
	struct file *fptr;
	struct fs_direntry direntry;
	int8_t str[30];
	
	hf_dev_ioctl(&ramdisk0, DISK_INIT, (void *)50);
	hf_dev_ioctl(&ramdisk0, DISK_GETINFO, (void *)&ramdisk0info);
	
	hf_mkfs(&ramdisk0, ramdisk0info.bytes_sector);
	hf_mount(&ramdisk0);
	hf_getlabel(&ramdisk0, str);
	printf("\nfreeblks: %d, label: %s", hf_getfree(&ramdisk0), str);
	
	hf_mkdir(&ramdisk0, "/uhhh");
	hf_mkdir(&ramdisk0, "/tchub");
	hf_mkdir(&ramdisk0, "/mu");
	hf_mkdir(&ramdisk0, "/mu2");
	hf_mkdir(&ramdisk0, "/mu3");
	hf_mkdir(&ramdisk0, "/mu4");
	hf_mkdir(&ramdisk0, "/mu5");
	hf_mkdir(&ramdisk0, "/uhhh/oler");
	hf_mkdir(&ramdisk0, "/uhhh/olar");
	hf_mkdir(&ramdisk0, "/uhhh/oler/22");
	hf_mkdir(&ramdisk0, "/uhhh/oler/22/putchu");
	hf_mkdir(&ramdisk0, "/uhhh/olar/putchu");
	hf_mkdir(&ramdisk0, "/uhhh/olar/putchu/33");
	hf_mkdir(&ramdisk0, "/mu/oper");


	hf_mkdir(&ramdisk0, "/mu5/1");
	hf_mkdir(&ramdisk0, "/mu5/2");
	hf_mkdir(&ramdisk0, "/mu5/3");
	hf_mkdir(&ramdisk0, "/mu5/4");
	hf_mkdir(&ramdisk0, "/mu5/5");
	
	hf_mkdir(&ramdisk0, "/mu5/3/1");
	hf_mkdir(&ramdisk0, "/mu5/3/2");
	hf_mkdir(&ramdisk0, "/mu5/3/3");
	hf_mkdir(&ramdisk0, "/mu5/3/4");
	hf_mkdir(&ramdisk0, "/mu5/3/5");
	
	hf_mkdir(&ramdisk0, "/mu5/3/1/0");
	hf_mkdir(&ramdisk0, "/mu5/3/5/666");

	fptr = hf_opendir(&ramdisk0, "/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}

	hf_umount(&ramdisk0);
//	hf_dev_ioctl(&ramdisk0, DISK_FINISH, 0);
}
