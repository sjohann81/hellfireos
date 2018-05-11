#include <hellfire.h>
#include <device.h>
#include <block.h>
#include <uhfs.h>

/* auxiliary functions */
static int32_t ispowerof2(uint32_t x){
	return x && !(x & (x - 1));
}

static uint32_t getfreeblock(struct device *dev)
{
	struct fs_blkdevice *blk_device;
	uint32_t j, chain_blk;
	
	blk_device = dev->ptr;
	
	/* find a free block, starting from the first cluster map block */
	chain_blk = blk_device->fssblock.first_cmb;
	while (1) {
		if (chain_blk == UHFS_EOCHBLK) {
#if UHFS_DEBUG == 1
			kprintf("\ngetfreeblock: storage device is full");
#endif
			return 0;
		}
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		for (j = 1; j < blk_device->fssblock.block_size / sizeof(uint32_t); j++)
			if (blk_device->datablock.cmb_data[j] == UHFS_FREEBLK) break;

		if (j < blk_device->fssblock.block_size / sizeof(uint32_t)) break;
						
		chain_blk = blk_device->datablock.cmb_data[0];
	}
#if UHFS_DEBUG == 1
	kprintf("\nfree blk at %d", chain_blk + j);
#endif			
	/* update the cluster map block */
	blk_device->datablock.cmb_data[j] = UHFS_EOCHBLK;
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
	hf_dev_write(dev, blk_device->datablock.cmb_data, 1);
	
	return chain_blk + j;
}

static int32_t searchdirectory(struct device *dev, int8_t *path, uint32_t *pblock, int8_t **ppath, uint32_t *lblock, int8_t **lpath)
{
	struct fs_blkdevice *blk_device;
	int32_t found;
	uint32_t i, chain_blk, dir_blk, dir_blk_next, first_dir_blk;

	
	blk_device = dev->ptr;
	path = strtok(path, " /");

	if (!path) {
#if UHFS_DEBUG == 1
		kprintf("\nsearchdirectory: path not found");
#endif
		return -1;
	}
	
	/* search the path, following the directory tree */
	chain_blk = blk_device->fssblock.first_cmb;
	dir_blk = blk_device->fssblock.root_dir_block;
	first_dir_blk = dir_blk;
	*pblock = 0;
	while (path != NULL) {
		found = 0;
		do {
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
			hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
			dir_blk_next = blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
#if UHFS_DEBUG == 1
			kprintf("\nchain %d dir_blk_next: %d", chain_blk, dir_blk_next);
#endif
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
			hf_dev_read(dev, blk_device->datablock.dir_data, 1);

			for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry) && !found; i++) {
				if (!(blk_device->datablock.dir_data[i].attributes & UHFS_ATTRFREE)) {
					if (strcmp(blk_device->datablock.dir_data[i].filename, path) == 0) {
						if (blk_device->datablock.dir_data[i].attributes & UHFS_ATTRDIR) {
							found = 1;
							*pblock = dir_blk;
							*ppath = path;
							dir_blk = blk_device->datablock.dir_data[i].first_block;
							first_dir_blk = dir_blk;
						} else {
#if UHFS_DEBUG == 1
							kprintf("\nsearchdirectory: %s is not a directory", blk_device->datablock.dir_data[i].filename);
#endif
							*pblock = dir_blk;
							*ppath = path;
							*lblock = blk_device->datablock.dir_data[i].first_block;
							*lpath = path;

							return 1;
						}
					}
				}
			}
			if (found) break;

			chain_blk = ((dir_blk_next - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
			dir_blk = dir_blk_next;
			if (dir_blk < 0 || (dir_blk > blk_device->fssblock.n_blocks * blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) break;
			
		} while (dir_blk_next != UHFS_EOCHBLK);
		
		*lpath = path;
		path = strtok(NULL, " /");
		if (!found) {
			if (path) {
#if UHFS_DEBUG == 1
				kprintf("\nsearchdirectory: path not found");
#endif
				return -1;
			}
			break;
			
		}
	}
	
	if (found) {
#if UHFS_DEBUG == 1
		kprintf("\nsearchdirectory: file/directory already exists");
#endif
		return -1;
	}
	
	*lblock = first_dir_blk;
	
	return 0;
}

/* volume management */
int32_t hf_mkfs(struct device *dev, uint32_t blk_size)
{
	struct blk_info fsblk_info;
	struct fs_blkdevice blk_device;
	uint32_t i, k;

	if (dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mkfs: can't do this on a mounted filesystem");
#endif
		return -1;
	}

	hf_dev_ioctl(dev, DISK_GETINFO, &fsblk_info);
	if (!ispowerof2(fsblk_info.bytes_sector)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mkfs: invalid sector size");
#endif
		return -1;
	}
	if (!ispowerof2(blk_size) || (blk_size < fsblk_info.bytes_sector)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mkfs: invalid block size");
#endif
		return -1;
	}
#if UHFS_DEBUG == 1
	kprintf("\nhf_mkfs: device has %d sectors (%d bytes each), block size: %d", fsblk_info.num_sectors, fsblk_info.bytes_sector, blk_size);
#endif
	/* allocate file system data structures and attach to device */
	blk_device.fsblk_info = fsblk_info;
	blk_device.vsize = fsblk_info.num_sectors * fsblk_info.bytes_sector;
	
	blk_device.fssblock.signature = 0x66600666;
	strncpy(blk_device.fssblock.oem_id, "uhfs_uhfs_uhfs", sizeof(blk_device.fssblock.oem_id));
	blk_device.fssblock.block_size = blk_size;
	blk_device.fssblock.n_blocks = blk_device.vsize / blk_size;
	strncpy(blk_device.fssblock.volume_label, "new volume", sizeof(blk_device.fssblock.volume_label));
	blk_device.fssblock.vdate.day = 1;
	blk_device.fssblock.vdate.month = 1;
	blk_device.fssblock.vdate.year = 1980;
	blk_device.fssblock.vdate.reserved = 0;
	blk_device.fssblock.vtime.millisecond = 0;
	blk_device.fssblock.vtime.second = 0;
	blk_device.fssblock.vtime.minute = 0;
	blk_device.fssblock.vtime.hour = 0;
	blk_device.fssblock.vtime.reserved = 0;
	blk_device.fssblock.first_cmb = 1;
	blk_device.fssblock.root_dir_block = 2;
	blk_device.fssblock.metadata_block = 0;
		
	blk_device.datablock.data = (int8_t *)hf_malloc(blk_size);
	if (!blk_device.datablock.data) return -1;
	
	/* write the superblock */
	memcpy(blk_device.datablock.data, &blk_device.fssblock, sizeof(struct fs_superblock));
	memset(blk_device.datablock.data + sizeof(struct fs_superblock), 0, blk_size - sizeof(struct fs_superblock));
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)0);
	hf_dev_write(dev, blk_device.datablock.data, 1);
	
	/* write cluster map blocks and data blocks (storage regions) */
	for (k = 1; k < blk_device.fssblock.n_blocks; k += blk_size / (sizeof(uint32_t))) {
		/* set pointer to the next cluster map block or end of chain */
		if ((k + blk_size / sizeof(uint32_t)) * blk_size < blk_device.vsize)
			blk_device.datablock.cmb_data[0] = k + blk_size / sizeof(uint32_t);
		else
			blk_device.datablock.cmb_data[0] = UHFS_EOCHBLK;

		/* fill the cluster map block with free or fixed blocks pointers */
		for (i = 1; i < blk_size / sizeof(uint32_t); i++) {
			if ((k - 1 + i) * blk_size < blk_device.vsize - blk_size)
				blk_device.datablock.cmb_data[i] = UHFS_FREEBLK;
			else
				blk_device.datablock.cmb_data[i] = UHFS_FIXDBLK;
		}
		/* write cluster map block from this storage region */
		hf_dev_write(dev, blk_device.datablock.data, 1);
		
		/* fill data with zeroes and write blocks of this storage region to disk */
		memset(blk_device.datablock.data, 0, blk_size);
		for (i = 1; i < blk_size / sizeof(uint32_t); i++)
			if ((k - 1 + i) * blk_size < blk_device.vsize - blk_size)
				hf_dev_write(dev, blk_device.datablock.data, 1);
	}
	
	/* create root directory */
	memset(&blk_device.fsdirentry, 0, sizeof(struct fs_direntry));
	blk_device.fsdirentry.attributes = UHFS_ATTRFREE;
	for (i = 0; i < blk_size / sizeof(struct fs_direntry); i++)
		memcpy(blk_device.datablock.data + i * sizeof(struct fs_direntry), &blk_device.fsdirentry, sizeof(struct fs_direntry));
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)blk_device.fssblock.root_dir_block);
	hf_dev_write(dev, blk_device.datablock.data, 1);
	
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)blk_device.fssblock.first_cmb);
	hf_dev_read(dev, blk_device.datablock.data, 1);
	blk_device.datablock.cmb_data[blk_device.fssblock.first_cmb] = UHFS_EOCHBLK;
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)blk_device.fssblock.first_cmb);
	hf_dev_write(dev, blk_device.datablock.data, 1);
	
	hf_free(blk_device.datablock.data);
	
	return 0;
}

int32_t hf_mount(struct device *dev)
{
	struct blk_info fsblk_info;
	struct fs_blkdevice *blk_device;
	struct fs_superblock *tmp_sblock;

	if (dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mount: filesystem already mounted");
#endif
		return -1;
	}

	hf_dev_ioctl(dev, DISK_GETINFO, &fsblk_info);
	if (!ispowerof2(fsblk_info.bytes_sector)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mount: invalid sector size");
#endif
		return -1;
	}
	
	/* allocate file system data structures */
	blk_device = (struct fs_blkdevice *)hf_malloc(sizeof(struct fs_blkdevice));
	if (!blk_device) return -1;
	blk_device->fsblk_info = fsblk_info;
	blk_device->vsize = fsblk_info.num_sectors * fsblk_info.bytes_sector;
	
	/* read superblock from the first media sector. FIXME: maybe read other copies if this fails? */
	tmp_sblock = (struct fs_superblock *)hf_malloc(fsblk_info.bytes_sector);
	if (!tmp_sblock) return -1;
	
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)0);
	hf_dev_read(dev, tmp_sblock, 1);
	memcpy(&blk_device->fssblock, tmp_sblock, sizeof(struct fs_superblock));
	hf_free(tmp_sblock);
	
	/* allocate memory for a block of data. this is the only block we have in memory! */
	if (!ispowerof2(blk_device->fssblock.block_size) || (blk_device->fssblock.block_size < fsblk_info.bytes_sector)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mount: invalid block size");
#endif
		return -1;
	}
	blk_device->datablock.data = (int8_t *)hf_malloc(blk_device->fssblock.block_size);
	if (!blk_device->datablock.data) return -1;
	
	/* attach filesystem structure (fs_blkdevice) to device */
	dev->ptr = blk_device;
#if UHFS_DEBUG == 1
	kprintf("\nhf_mount: block device mounted; sector size: %d, sectors %d, block size: %d, blocks: %d", 
		blk_device->fsblk_info.bytes_sector, blk_device->fsblk_info.num_sectors, blk_device->fssblock.block_size, blk_device->fssblock.n_blocks); 
#endif
	return 0;
}

int32_t hf_umount(struct device *dev)
{
	struct fs_blkdevice *blk_device;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_umount: filesystem not mounted");
#endif
		return -1;
	}
	
	/* free data structures from device: data block and block device structure */
	blk_device = dev->ptr;
	hf_free(blk_device->datablock.data);
	hf_free(blk_device);
	
	/* detach filesystem from device */
	dev->ptr = 0;
#if UHFS_DEBUG == 1
	kprintf("\nhf_umount: block device unmounted");
#endif
	return 0;
}

int32_t hf_getfree(struct device *dev)
{
	struct fs_blkdevice *blk_device;
	uint32_t k, freeblks = 0;

	if (!dev->ptr) return -1;
	
	blk_device = dev->ptr;
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)blk_device->fssblock.first_cmb);
	
	/* sweep through all cluster map blocks, counting free blocks */
	while (1) {
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		
		for (k = 1; k < blk_device->fssblock.block_size / sizeof(uint32_t); k++)
			if (blk_device->datablock.cmb_data[k] == UHFS_FREEBLK)
				freeblks++;
		
		if (blk_device->datablock.cmb_data[0] == UHFS_EOCHBLK)
			break;
			
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)blk_device->datablock.cmb_data[0]);
	};
	
	return freeblks;
}

int32_t hf_getlabel(struct device *dev, int8_t *label)
{
	struct fs_blkdevice *blk_device;
	
	blk_device = dev->ptr;
	strncpy(label, blk_device->fssblock.volume_label, sizeof(blk_device->fssblock.volume_label));
	
	return 0;
}

int32_t hf_setlabel(struct device *dev, int8_t *label)
{
	struct fs_blkdevice *blk_device;
	
	blk_device = dev->ptr;
	strncpy(blk_device->fssblock.volume_label, label, sizeof(blk_device->fssblock.volume_label));
	
	return 0;
}


/* directory management / operations */
int32_t hf_mkdir(struct device *dev, int8_t *path)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, j, k, chain_blk, chain_blk_last, dir_blk, dir_blk_next, dir_blk_last, parent_dir_blk, first_dir_blk;
	int8_t *ppath, *lpath;
	int8_t *dirpath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_mkdir: filesystem not mounted");
#endif
		return -1;
	}
	
	dirpath = (int8_t *)hf_malloc(strlen(path));
	if (!dirpath)
		return -1;
	strcpy(dirpath, path);

	if (searchdirectory(dev, dirpath, &parent_dir_blk, &ppath, &first_dir_blk, &lpath)) {
		kprintf("\nhf_mkdir: path not found");
		hf_free(dirpath);
		return -1;
	}
#if UHFS_DEBUG == 1
	kprintf("\npath / name is ok, now do it");
#endif
	blk_device = dev->ptr;
	
	/* find the first empty directory entry */
	chain_blk = ((first_dir_blk - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	dir_blk = first_dir_blk;
#if UHFS_DEBUG == 1
	kprintf("\ndirectory at blk %d (pblock %d)", dir_blk, parent_dir_blk);
#endif
	while (1) {
		do {
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
			hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
			dir_blk_next = blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
			hf_dev_read(dev, blk_device->datablock.dir_data, 1);
#if UHFS_DEBUG == 1
			kprintf("\nhf_mkdir: chain: %d blk: %d next %d", chain_blk, dir_blk, dir_blk_next);
#endif
			for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
				if (blk_device->datablock.dir_data[i].attributes & UHFS_ATTRFREE) {
					/* get a free block, update the cluster map */
					k = getfreeblock(dev);
					if (!k) return -1;
					
					/* clean the block for empty directory entries */
					hf_dev_ioctl(dev, DISK_SEEKSET, (void *)k);
					memset(blk_device->datablock.dir_data, 0, blk_device->fssblock.block_size);
					for (j = 0; j < blk_device->fssblock.block_size / sizeof(struct fs_direntry); j++)
						blk_device->datablock.dir_data[j].attributes = UHFS_ATTRFREE;
					hf_dev_write(dev, blk_device->datablock.dir_data, 1);
					
					/* update the directory entry, pointing to the new subdirectory file */
					hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
					hf_dev_read(dev, blk_device->datablock.dir_data, 1);

					strcpy(blk_device->datablock.dir_data[i].filename, lpath);
					blk_device->datablock.dir_data[i].attributes = UHFS_ATTRDIR | UHFS_ATTRREAD | UHFS_ATTRWRITE | UHFS_ATTREXEC;
					blk_device->datablock.dir_data[i].metadata_block = 0;
					blk_device->datablock.dir_data[i].first_block = k;
					blk_device->datablock.dir_data[i].size = 0;
					hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
					hf_dev_write(dev, blk_device->datablock.dir_data, 1);
					
					hf_free(dirpath);
					
					return 0;
				}
			}
			
			chain_blk_last = chain_blk;
			chain_blk = ((dir_blk_next - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
			dir_blk_last = dir_blk;
			dir_blk = dir_blk_next;
#if UHFS_DEBUG == 1
			kprintf("\nhf_mkdir: jumping to next part of the directory (%d)", dir_blk_next);
#endif
		} while (dir_blk_next != UHFS_EOCHBLK);

		/* we should create an extension of the directory file and not give up here */
#if UHFS_DEBUG == 1
		kprintf("\nhf_mkdir: directory is full. extending it...");
#endif
		k = getfreeblock(dev);
		if (!k){
			hf_free(dirpath);
			
			return -1;
		}
			
		/* clean the block for empty directory entries */
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)k);
		memset(blk_device->datablock.dir_data, 0, blk_device->fssblock.block_size);
		for (j = 0; j < blk_device->fssblock.block_size / sizeof(struct fs_direntry); j++)
			blk_device->datablock.dir_data[j].attributes = UHFS_ATTRFREE;
		hf_dev_write(dev, blk_device->datablock.dir_data, 1);

		/* update the a cluster map block */
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk_last);
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		blk_device->datablock.cmb_data[(dir_blk_last - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)] = k;
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk_last);
		hf_dev_write(dev, blk_device->datablock.cmb_data, 1);
		
		dir_blk = k;
		chain_blk = ((k - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	}
	
	hf_free(dirpath);
	
	return 0;
}

struct file * hf_opendir(struct device *dev, int8_t *path)
{
	struct file *fptr;
	uint32_t parent_dir_blk, first_dir_blk;
	int8_t *ppath, *lpath;
	int8_t *dirpath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_opendir: filesystem not mounted");
#endif
		return 0;
	}

	fptr = (struct file *)hf_malloc(sizeof(struct file));
	if (!fptr)
		return 0;

	dirpath = (int8_t *)hf_malloc(strlen(path));
	if (!dirpath)
		return 0;
	strcpy(dirpath, path);	
	
	if (searchdirectory(dev, dirpath, &parent_dir_blk, &ppath, &first_dir_blk, &lpath)) {
		hf_free(fptr);
#if UHFS_DEBUG == 1
		kprintf("\nhf_opendir: path not found");
#endif
		hf_free(dirpath);
		return 0;
	}
	
	fptr->dev = dev;
	fptr->first_block = first_dir_blk;
	fptr->mode = 0;			// should match the actual file mode! check hf_chmod()
	fptr->flags = UHFS_OPENFILE;
	fptr->block = first_dir_blk;
	fptr->offset = 0;
	
	hf_free(dirpath);
	
	return fptr;
}

int32_t hf_closedir(struct file *desc)
{
	if (!(desc->flags & UHFS_OPENFILE)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_closedir: not an open directory");
#endif
		return -1;
	}

	desc->flags = 0;
	hf_free(desc);
	
	return 0;
}

int32_t hf_readdir(struct file *desc, struct fs_direntry *entry)
{
	struct fs_blkdevice *blk_device;
	static uint32_t chain_blk, dir_blk, dir_blk_next;
	
	if (!(desc->flags & UHFS_OPENFILE)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_readdir: not an open directory");
#endif
		return -1;
	}

	if (desc->flags & UHFS_EOF)
		return -1;
	
	blk_device = desc->dev->ptr;

	if (desc->offset == 0){
		/* find the next directory block */
		chain_blk = ((desc->block - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
		dir_blk = desc->block;

		hf_dev_ioctl(desc->dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_read(desc->dev, blk_device->datablock.cmb_data, 1);
		dir_blk_next = blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
		hf_dev_ioctl(desc->dev, DISK_SEEKSET, (void *)dir_blk);
		hf_dev_read(desc->dev, blk_device->datablock.dir_data, 1);
		desc->block = dir_blk_next;
#if UHFS_DEBUG == 1
		kprintf("\nhf_readdir: chain: %d blk: %d next %d", chain_blk, dir_blk, dir_blk_next);
#endif
	}

	if (desc->block == UHFS_EOCHBLK && desc->offset == (blk_device->fssblock.block_size / sizeof(struct fs_direntry)) - 1)
		desc->flags |= UHFS_EOF;

	*entry = blk_device->datablock.dir_data[desc->offset++];
	if (desc->offset >= (blk_device->fssblock.block_size / sizeof(struct fs_direntry)))
		desc->offset = 0;
	
	return 0;
}

int32_t hf_rmdir(struct device *dev, int8_t *path)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, chain_blk, dir_blk, dir_blk_next, parent_dir_blk, first_dir_blk;
	int8_t *ppath, *lpath;
	int8_t *dirpath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_rmdir: filesystem not mounted");
#endif
		return -1;
	}
	
	dirpath = (int8_t *)hf_malloc(strlen(path));
	if (!dirpath)
		return -1;
	strcpy(dirpath, path);

	if (searchdirectory(dev, dirpath, &parent_dir_blk, &ppath, &first_dir_blk, &lpath)) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_rmdir: path not found");
#endif
		hf_free(dirpath);
		
		return -1;
	}
#if UHFS_DEBUG == 1
	kprintf("\npath / name is ok, now do it");
#endif
	blk_device = dev->ptr;
	
	chain_blk = ((first_dir_blk - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	dir_blk = first_dir_blk;
#if UHFS_DEBUG == 1
	kprintf("\ndirectory at blk %d (pblock %d)", dir_blk, parent_dir_blk);
#endif
	/* find a non-empty directory entry */
	do {
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		dir_blk_next = blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
		hf_dev_read(dev, blk_device->datablock.dir_data, 1);
#if UHFS_DEBUG == 1
		kprintf("\nhf_rmdir: chain: %d blk: %d next %d", chain_blk, dir_blk, dir_blk_next);
#endif
		for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
			/* directory is not empty */
			if (!(blk_device->datablock.dir_data[i].attributes & UHFS_ATTRFREE)){
#if UHFS_DEBUG == 1
				kprintf("\nhf_rmdir: directory not empty");
#endif
				hf_free(dirpath);
				
				return -1;
			}
		}

		chain_blk = ((dir_blk_next - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
		dir_blk = dir_blk_next;
#if UHFS_DEBUG == 1
		kprintf("\nhf_rmdir: jumping to next part of the directory (%d)", dir_blk_next);
#endif
	} while (dir_blk_next != UHFS_EOCHBLK);
#if UHFS_DEBUG == 1
	kprintf("\nok, directory '%s' is empty. parent is on block %d", ppath, parent_dir_blk);
#endif
	/* free directory entry on parent block */
	dir_blk = parent_dir_blk;

	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
	hf_dev_read(dev, blk_device->datablock.dir_data, 1);
	for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
		if (!strcmp(blk_device->datablock.dir_data[i].filename, ppath)){
			blk_device->datablock.dir_data[i].attributes |= UHFS_ATTRFREE;
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
			hf_dev_write(dev, blk_device->datablock.dir_data, 1);
#if UHFS_DEBUG == 1
			kprintf("\nhf_rmdir: freed directory entry");
#endif
			break;
		}
	}

	/* free directory blocks. FIXME: should avoid multiple writes to the same cluster map block */	
	chain_blk = ((first_dir_blk - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	dir_blk = first_dir_blk;
#if UHFS_DEBUG == 1
	kprintf("\nhf_rmdir: chain %d block %d", chain_blk, dir_blk);
#endif
	do {
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		dir_blk_next = blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
		blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)] = UHFS_FREEBLK;
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_write(dev, blk_device->datablock.cmb_data, 1);
#if UHFS_DEBUG == 1
		kprintf("\nhf_rmdir: freed block %d", dir_blk);
#endif
		chain_blk = ((dir_blk_next - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
		dir_blk = dir_blk_next;
	} while (dir_blk_next != UHFS_EOCHBLK);
	
	hf_free(dirpath);
	
	return 0;
}

/* file management */
int32_t hf_create(struct device *dev, int8_t *path)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, j, k, chain_blk, chain_blk_last, dir_blk, dir_blk_next, dir_blk_last, parent_dir_blk, first_dir_blk;
	int8_t *ppath, *lpath;
	int8_t *dirpath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_create: filesystem not mounted");
#endif
		return -1;
	}
	
	dirpath = (int8_t *)hf_malloc(strlen(path));
	if (!dirpath)
		return -1;
	strcpy(dirpath, path);

	if (searchdirectory(dev, dirpath, &parent_dir_blk, &ppath, &first_dir_blk, &lpath)) {
		kprintf("\nhf_create: path not found");
		hf_free(dirpath);
		return -1;
	}
#if UHFS_DEBUG == 1
	kprintf("\npath / name is ok, now do it");
#endif
	blk_device = dev->ptr;
	
	/* find the first empty directory entry */
	chain_blk = ((first_dir_blk - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	dir_blk = first_dir_blk;
#if UHFS_DEBUG == 1
	kprintf("\ndirectory at blk %d (pblock %d)", dir_blk, parent_dir_blk);
#endif
	while (1) {
		do {
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
			hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
			dir_blk_next = blk_device->datablock.cmb_data[(dir_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
			hf_dev_read(dev, blk_device->datablock.dir_data, 1);
#if UHFS_DEBUG == 1
			kprintf("\nhf_mkdir: chain: %d blk: %d next %d", chain_blk, dir_blk, dir_blk_next);
#endif
			for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
				if (blk_device->datablock.dir_data[i].attributes & UHFS_ATTRFREE) {
					/* get a free block, update the cluster map */
					k = getfreeblock(dev);
					if (!k) return -1;
					
					/* clean the block */
					hf_dev_ioctl(dev, DISK_SEEKSET, (void *)k);
					memset(blk_device->datablock.data, 0, blk_device->fssblock.block_size);
					hf_dev_write(dev, blk_device->datablock.dir_data, 1);
					
					/* update the directory entry, pointing to the new file */
					hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
					hf_dev_read(dev, blk_device->datablock.dir_data, 1);

					strcpy(blk_device->datablock.dir_data[i].filename, lpath);
					blk_device->datablock.dir_data[i].attributes = UHFS_ATTRREAD | UHFS_ATTRWRITE;
					blk_device->datablock.dir_data[i].metadata_block = 0;
					blk_device->datablock.dir_data[i].first_block = k;
					blk_device->datablock.dir_data[i].size = 0;
					hf_dev_ioctl(dev, DISK_SEEKSET, (void *)dir_blk);
					hf_dev_write(dev, blk_device->datablock.dir_data, 1);
					
					hf_free(dirpath);
					
					return 0;
				}
			}
			
			chain_blk_last = chain_blk;
			chain_blk = ((dir_blk_next - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
			dir_blk_last = dir_blk;
			dir_blk = dir_blk_next;
#if UHFS_DEBUG == 1
			kprintf("\nhf_create: jumping to next part of the directory (%d)", dir_blk_next);
#endif
		} while (dir_blk_next != UHFS_EOCHBLK);

		/* we should create an extension of the directory file and not give up here */
#if UHFS_DEBUG == 1
		kprintf("\nhf_create: directory is full. extending it...");
#endif
		k = getfreeblock(dev);
		if (!k){
			hf_free(dirpath);
			
			return -1;
		}
			
		/* clean the block for empty directory entries */
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)k);
		memset(blk_device->datablock.dir_data, 0, blk_device->fssblock.block_size);
		for (j = 0; j < blk_device->fssblock.block_size / sizeof(struct fs_direntry); j++)
			blk_device->datablock.dir_data[j].attributes = UHFS_ATTRFREE;
		hf_dev_write(dev, blk_device->datablock.dir_data, 1);

		/* update the a cluster map block */
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk_last);
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		blk_device->datablock.cmb_data[(dir_blk_last - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)] = k;
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk_last);
		hf_dev_write(dev, blk_device->datablock.cmb_data, 1);
		
		dir_blk = k;
		chain_blk = ((k - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	}
	
	hf_free(dirpath);
	
	return 0;
}

int32_t hf_unlink(struct device *dev, int8_t *path)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, chain_blk, file_blk, file_blk_next, parent_dir_blk, first_file_blk, ret;
	int8_t *ppath, *lpath;
	int8_t *filepath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_unlink: filesystem not mounted");
#endif
		return -1;
	}
	
	filepath = (int8_t *)hf_malloc(strlen(path));
	if (!filepath)
		return -1;
	strcpy(filepath, path);

	ret = searchdirectory(dev, filepath, &parent_dir_blk, &ppath, &first_file_blk, &lpath);
	switch (ret) {
	case 0:
#if UHFS_DEBUG == 1
		kprintf("\nhf_unlink: %s is a directory", path);
#endif
		hf_free(filepath);
		
		return -1;
	case 1:
#if UHFS_DEBUG == 1
		kprintf("\npath / name is ok, now do it");
#endif
		break;
	default:
#if UHFS_DEBUG == 1
		kprintf("\nhf_unlink: path not found");
#endif
		hf_free(filepath);
		
		return -1;
	}
	
	blk_device = dev->ptr;
	
	chain_blk = ((first_file_blk - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	file_blk = first_file_blk;
#if UHFS_DEBUG == 1
	kprintf("\nfile at blk %d (pblock %d)", file_blk, parent_dir_blk);
#endif
	/* free directory entry on parent block */
	file_blk = parent_dir_blk;

	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
	hf_dev_read(dev, blk_device->datablock.dir_data, 1);
	for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
		if (!strcmp(blk_device->datablock.dir_data[i].filename, ppath)){
			blk_device->datablock.dir_data[i].attributes |= UHFS_ATTRFREE;
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
			hf_dev_write(dev, blk_device->datablock.dir_data, 1);
#if UHFS_DEBUG == 1
			kprintf("\nhf_unlink: freed directory entry");
#endif
			break;
		}
	}

	/* free file blocks. FIXME: should avoid multiple writes to the same cluster map block */	
	chain_blk = ((first_file_blk - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
	file_blk = first_file_blk;
#if UHFS_DEBUG == 1
	kprintf("\nhf_unlink: chain %d block %d", chain_blk, file_blk);
#endif
	do {
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_read(dev, blk_device->datablock.cmb_data, 1);
		file_blk_next = blk_device->datablock.cmb_data[(file_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)];
		blk_device->datablock.cmb_data[(file_blk - 1) & (blk_device->fssblock.block_size / sizeof(uint32_t) - 1)] = UHFS_FREEBLK;
		hf_dev_ioctl(dev, DISK_SEEKSET, (void *)chain_blk);
		hf_dev_write(dev, blk_device->datablock.cmb_data, 1);
#if UHFS_DEBUG == 1
		kprintf("\nhf_unlink: freed block %d", file_blk);
#endif
		chain_blk = ((file_blk_next - 1) & ~(blk_device->fssblock.block_size / sizeof(uint32_t) - 1)) + 1;
		file_blk = file_blk_next;
	} while (file_blk_next != UHFS_EOCHBLK);
	
	hf_free(filepath);
	
	return 0;
}

int64_t hf_size(struct device *dev, int8_t *path)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, file_blk, parent_dir_blk, first_file_blk;
	int8_t *ppath, *lpath;
	int8_t *filepath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_size: filesystem not mounted");
#endif
		return -1;
	}
	
	filepath = (int8_t *)hf_malloc(strlen(path));
	if (!filepath)
		return -1;
	strcpy(filepath, path);

	if (searchdirectory(dev, filepath, &parent_dir_blk, &ppath, &first_file_blk, &lpath) != 1) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_size: path not found");
#endif
		hf_free(filepath);
		
		return -1;
	}

	blk_device = dev->ptr;
	/* find directory entry on parent block */
	file_blk = parent_dir_blk;
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
	hf_dev_read(dev, blk_device->datablock.dir_data, 1);
	for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
		if (!strcmp(blk_device->datablock.dir_data[i].filename, ppath)){
			hf_free(filepath);
			
			return blk_device->datablock.dir_data[i].size;
		}
	}	
	
	hf_free(filepath);
	
	return 0;
}

int32_t hf_rename(struct device *dev, int8_t *path, int8_t *newname)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, file_blk, parent_dir_blk, first_file_blk;
	int8_t *ppath, *lpath;
	int8_t *filepath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_size: filesystem not mounted");
#endif
		return -1;
	}
	
	filepath = (int8_t *)hf_malloc(strlen(path));
	if (!filepath)
		return -1;
	strcpy(filepath, path);

	if (searchdirectory(dev, filepath, &parent_dir_blk, &ppath, &first_file_blk, &lpath) != 1) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_size: path not found");
#endif
		hf_free(filepath);
		
		return -1;
	}

	blk_device = dev->ptr;
	/* find directory entry on parent block */
	file_blk = parent_dir_blk;
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
	hf_dev_read(dev, blk_device->datablock.dir_data, 1);
	for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
		if (!strcmp(blk_device->datablock.dir_data[i].filename, ppath)){
			strncpy(blk_device->datablock.dir_data[i].filename, newname, sizeof(blk_device->datablock.dir_data[i].filename));
			
			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
			hf_dev_write(dev, blk_device->datablock.dir_data, 1);						
			
			hf_free(filepath);
			
			return 0;
		}
	}	
	
	hf_free(filepath);

	return 0;
}

int32_t hf_chmod(struct device *dev, int8_t *path, int8_t mode)
{
	struct fs_blkdevice *blk_device;
	uint32_t i, file_blk, parent_dir_blk, first_file_blk;
	int8_t *ppath, *lpath;
	int8_t *filepath;
	
	if (!dev->ptr) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_size: filesystem not mounted");
#endif
		return -1;
	}
	
	filepath = (int8_t *)hf_malloc(strlen(path));
	if (!filepath)
		return -1;
	strcpy(filepath, path);

	if (searchdirectory(dev, filepath, &parent_dir_blk, &ppath, &first_file_blk, &lpath) != 1) {
#if UHFS_DEBUG == 1
		kprintf("\nhf_size: path not found");
#endif
		hf_free(filepath);
		
		return -1;
	}

	blk_device = dev->ptr;
	/* find directory entry on parent block */
	file_blk = parent_dir_blk;
	hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
	hf_dev_read(dev, blk_device->datablock.dir_data, 1);
	for (i = 0; i < blk_device->fssblock.block_size / sizeof(struct fs_direntry); i++) {
		if (!strcmp(blk_device->datablock.dir_data[i].filename, ppath)){
			blk_device->datablock.dir_data[i].attributes &= ~0x7e;		/* directory and free attributes cannot be changed */
			blk_device->datablock.dir_data[i].attributes |= mode;

			hf_dev_ioctl(dev, DISK_SEEKSET, (void *)file_blk);
			hf_dev_write(dev, blk_device->datablock.dir_data, 1);						
			
			hf_free(filepath);
			
			return 0;
		}
	}	
	
	hf_free(filepath);

	return 0;
}

int32_t hf_touch(struct device *dev, int8_t *path, struct fs_date *ndate, struct fs_time *ntime)
{
	return 0;
}

/* file operations */
struct file * hf_fopen(struct device *dev, int8_t *path, int8_t *mode)
{
	return 0;
}

int32_t hf_fclose(struct file *desc)
{
	return 0;
}

int64_t hf_fread(void *buf, int32_t isize, int32_t items, struct file *desc)
{
	return 0;
}

int64_t hf_fwrite(void *buf, int32_t isize, int32_t items, struct file *desc)
{
	return 0;
}

int32_t hf_fseek(struct file *desc, int64_t offset, int32_t whence)
{
	return 0;
}

int64_t hf_ftell(struct file *desc)
{
	return 0;
}

int32_t hf_feof(struct file *desc)
{
	return 0;
}
