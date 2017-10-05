#define UHFS_DEBUG	1

#define UHFS_FIXDBLK		0xfffffffc		/* fixed / not allocatable */
#define UHFS_DEADBLK		0xfffffffd		/* invalid (dead block) */
#define UHFS_EOCHBLK		0xfffffffe		/* last block in the chain (end of file or end of chain of cluster map blocks) */
#define UHFS_FREEBLK		0xffffffff		/* unused (free) block */

#define UHFS_ATTRDIR		0x01			/* directory */
#define UHFS_ATTRHIDDEN		0x02			/* hidden */
#define UHFS_ATTRARCHIVE	0x04			/* file needs archiving */
#define UHFS_ATTRSYSTEM		0x08			/* system */
#define UHFS_ATTREXEC		0x10			/* execute */
#define UHFS_ATTRWRITE		0x20			/* write */
#define UHFS_ATTRREAD		0x40			/* read */
#define UHFS_ATTRFREE		0x80			/* deleted / free directory entry */

#define UHFS_RDONLY		0x0001
#define UHFS_WRONLY		0x0002
#define UHFS_RDWR		0x0003
#define UHFS_CREAT		0x0004
#define UHFS_APPEND		0x0008
#define UHFS_SYNC		0x0010
#define UHFS_NONBLOCK		0x0020
#define UHFS_EOF		0x4000
#define UHFS_OPENFILE		0x8000

enum {
	UHFS_EOK		= 0,
	UHFS_EERROR		= -1000,
	UHFS_ENOTMOUNTED	= -1001,
	UHFS_EALREADYMOUNTED	= -1002,
	UHFS_EINVSECTORSIZE	= -1003,
	UHFS_EINVBLOCKSIZE	= -1004,
	UHFS_ESTORAGEFULL	= -1005,
	UHFS_EPATHNOTFOUND	= -1006,
	UHFS_ENOTADIR		= -1007,
	UHFS_ENOTAFILE		= -1008,
	UHFS_EFILEEXISTS	= -1009,
	UHFS_EFILENOTOPEN	= -1010,
	UHFS_EDIRNOTEMPTY	= -1011
};

struct fs_date {
	uint32_t day : 5;			/* range 1 - 31 */
	uint32_t month : 4;			/* range 1 - 12 */
	uint32_t year : 15;			/* range 0 - 32767 */
	uint32_t reserved : 8;
};

struct fs_time {
	uint32_t millisecond : 7;		/* precision is 0.0078125 seconds */
	uint32_t second : 6;			/* range 0 - 59 */
	uint32_t minute : 6;			/* range 0 - 59 */
	uint32_t hour : 5;			/* range 0 - 23 */
	uint32_t reserved : 8;
};

struct fs_superblock {
	uint32_t signature;
	int8_t oem_id[16];
	uint32_t block_size;
	uint32_t n_blocks;
	int8_t volume_label[16];
	struct fs_date vdate;
	struct fs_time vtime;
	uint32_t first_cmb;
	uint32_t root_dir_block;
	uint32_t metadata_block;
};

struct fs_direntry {
	int8_t		filename[39];		/* all bytes except / and NULL */
	uint8_t		attributes;
	struct fs_date	date;
	struct fs_time	time;
	uint32_t	metadata_block;		/* block containing metadata for this file (future use) */
	uint32_t	first_block;		/* the number of the first data block */
	uint64_t	size;
};

union fs_datablock {
	uint32_t *cmb_data;
	int8_t *data;
	struct fs_direntry *dir_data;
};

struct fs_blkdevice {
	/* these structures remain fixed after the filesystem is initialized */
	struct blk_info fsblk_info;
	uint64_t vsize;
	struct fs_superblock fssblock;
	/* these structures change during filesystem usage */
	struct fs_direntry fsdirentry;
	union fs_datablock datablock;
};

struct file {
	struct device *dev;
	uint32_t first_block;
	int8_t mode;
	int32_t flags;
	uint32_t block;
	int64_t offset;
};

/* volume management */
int32_t hf_mkfs(struct device *dev, uint32_t blk_size);
int32_t hf_mount(struct device *dev);
int32_t hf_umount(struct device *dev);
int32_t hf_getfree(struct device *dev);
int32_t hf_getlabel(struct device *dev, int8_t *label);
int32_t hf_setlabel(struct device *dev, int8_t *label);

/* directory management / operations */
int32_t hf_mkdir(struct device *dev, int8_t *path);
struct file * hf_opendir(struct device *dev, int8_t *path);
int32_t hf_closedir(struct file *desc);
int32_t hf_readdir(struct file *desc, struct fs_direntry *entry);
int32_t hf_rmdir(struct device *dev, int8_t *path);

/* file management */
int32_t hf_create(struct device *dev, int8_t *path);
int32_t hf_unlink(struct device *dev, int8_t *path);
int64_t hf_size(struct device *dev, int8_t *path);
int32_t hf_rename(struct device *dev, int8_t *path, int8_t *newname);
int32_t hf_chmod(struct device *dev, int8_t *path, int8_t mode);
int32_t hf_touch(struct device *dev, int8_t *path, struct fs_date *ndate, struct fs_time *ntime);

/* file operations */
struct file * hf_fopen(struct device *dev, int8_t *path, int8_t *mode);
int32_t hf_fclose(struct file *desc);
int64_t hf_fread(void *buf, int32_t isize, int32_t items, struct file *desc);
int64_t hf_fwrite(void *buf, int32_t isize, int32_t items, struct file *desc);
int32_t hf_fseek(struct file *desc, int64_t offset, int32_t whence);
int64_t hf_ftell(struct file *desc);
int32_t hf_feof(struct file *desc);
