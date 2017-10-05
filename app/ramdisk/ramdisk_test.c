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
	
	hf_mkdir(&ramdisk0, "/root");
/*	
	hf_mkdir(&ramdisk0, "/root/coisa");
	hf_mkdir(&ramdisk0, "/root/cao");
	hf_mkdir(&ramdisk0, "/root/mu");
	hf_mkdir(&ramdisk0, "/root/mu2");
	hf_mkdir(&ramdisk0, "/root/mu3");
	hf_mkdir(&ramdisk0, "/root/mu4");
	hf_mkdir(&ramdisk0, "/root/mu5");
	hf_mkdir(&ramdisk0, "/root/coisa/abcdef");
	hf_mkdir(&ramdisk0, "/root/coisa/fedcba");
	hf_mkdir(&ramdisk0, "/root/coisa/abcdef/22");
	hf_mkdir(&ramdisk0, "/root/coisa/abcdef/22/uuuuuuu");
	hf_mkdir(&ramdisk0, "/root/coisa/fedcba/uuuuuuu");
	hf_mkdir(&ramdisk0, "/root/coisa/fedcba/uuuuuuu/33");
	hf_mkdir(&ramdisk0, "/root/mu/oper");


	hf_mkdir(&ramdisk0, "/root/mu5/1");
	hf_mkdir(&ramdisk0, "/root/mu5/2");
	hf_mkdir(&ramdisk0, "/root/mu5/3");
	hf_mkdir(&ramdisk0, "/root/mu5/4");
	hf_mkdir(&ramdisk0, "/root/mu5/5");
	
	hf_mkdir(&ramdisk0, "/root/mu5/3/1");
	hf_mkdir(&ramdisk0, "/root/mu5/3/2");
	hf_mkdir(&ramdisk0, "/root/mu5/3/3");
	hf_mkdir(&ramdisk0, "/root/mu5/3/4");
	hf_mkdir(&ramdisk0, "/root/mu5/3/5");
	
	hf_mkdir(&ramdisk0, "/root/mu5/3/1/0");
	hf_mkdir(&ramdisk0, "/root/mu5/3/5/666");
	hf_mkdir(&ramdisk0, "/root/mu/12345678");
	hf_mkdir(&ramdisk0, "/root/mu/12345678/111");
	hf_mkdir(&ramdisk0, "/root/les");
	hf_mkdir(&ramdisk0, "/root/les2");
	hf_mkdir(&ramdisk0, "/root/les3");
	hf_mkdir(&ramdisk0, "/root/les4");
	hf_mkdir(&ramdisk0, "/root/les5");

	fptr = hf_opendir(&ramdisk0, "/root/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}
	hf_closedir(fptr);
	
	fptr = hf_opendir(&ramdisk0, "/root/coisa/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}
	hf_closedir(fptr);
	
	fptr = hf_opendir(&ramdisk0, "/root/mu5/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}
	hf_closedir(fptr);
	
	fptr = hf_opendir(&ramdisk0, "/root/coisa/abcdef/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}
	hf_closedir(fptr);
	
	fptr = hf_opendir(&ramdisk0, "/root/mu5/3/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}
	hf_closedir(fptr);

	hf_rmdir(&ramdisk0, "/root/mu5/3/5/666/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/1/0/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/5/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/4/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/3/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/2/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/1/.");
	
	hf_rmdir(&ramdisk0, "/root/mu5/5/.");
	hf_rmdir(&ramdisk0, "/root/mu5/4/.");
	hf_rmdir(&ramdisk0, "/root/mu5/3/.");
	hf_rmdir(&ramdisk0, "/root/mu5/2/.");
	hf_rmdir(&ramdisk0, "/root/mu5/1/.");
	
	hf_rmdir(&ramdisk0, "/root/mu/12345678/111/.");
	hf_rmdir(&ramdisk0, "/root/mu/12345678/.");

	hf_rmdir(&ramdisk0, "/root/coisa/abcdef/22/uuuuuuu/.");
	hf_rmdir(&ramdisk0, "/root/coisa/abcdef/22/.");
	hf_rmdir(&ramdisk0, "/root/coisa/fedcba/uuuuuuu/33/.");
	hf_rmdir(&ramdisk0, "/root/coisa/fedcba/uuuuuuu/.");
	hf_rmdir(&ramdisk0, "/root/coisa/abcdef/.");
	hf_rmdir(&ramdisk0, "/root/coisa/fedcba/.");
	
	hf_rmdir(&ramdisk0, "/root/coisa/.");
	hf_rmdir(&ramdisk0, "/root/cao/.");
	hf_rmdir(&ramdisk0, "/root/mu/oper/.");
	hf_rmdir(&ramdisk0, "/root/mu/.");
	hf_rmdir(&ramdisk0, "/root/mu2/.");
	hf_rmdir(&ramdisk0, "/root/mu3/.");
	hf_rmdir(&ramdisk0, "/root/mu4/.");
	hf_rmdir(&ramdisk0, "/root/mu5/.");
	
	hf_rmdir(&ramdisk0, "/root/les/.");
	hf_rmdir(&ramdisk0, "/root/les2/.");
	hf_rmdir(&ramdisk0, "/root/les3/.");
	hf_rmdir(&ramdisk0, "/root/les4/.");
	hf_rmdir(&ramdisk0, "/root/les5/.");
	
	fptr = hf_opendir(&ramdisk0, "/root/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
		if (direntry.attributes & UHFS_ATTRFREE)
			printf("*");
	}
	hf_closedir(fptr);
	
	hf_rmdir(&ramdisk0, "/root/.");
	
	hf_mkdir(&ramdisk0, "/root");
*/
	hf_create(&ramdisk0, "/root/pig");
	hf_create(&ramdisk0, "/root/pig2");
	hf_create(&ramdisk0, "/root/pig3");

	hf_unlink(&ramdisk0, "/root/pig2");
	printf("\nsize %d", hf_size(&ramdisk0, "/root/pig"));
	
	fptr = hf_opendir(&ramdisk0, "/root/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
	}
	hf_closedir(fptr);
	
	hf_rename(&ramdisk0, "/root/pig", "pig1");
	
	fptr = hf_opendir(&ramdisk0, "/root/.");
	while (!hf_readdir(fptr, &direntry)){
		
		printf("\nfile: %s", direntry.filename);
		if (direntry.attributes & UHFS_ATTRFREE) putchar('*');
	}
	hf_closedir(fptr);
	
	

	printf("\nfreeblks: %d, label: %s", hf_getfree(&ramdisk0), str);
	
	hf_umount(&ramdisk0);
	hf_dev_ioctl(&ramdisk0, DISK_FINISH, 0);
}
