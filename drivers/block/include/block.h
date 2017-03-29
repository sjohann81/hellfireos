enum {
	DISK_INIT = 100,
	DISK_GETINFO,
	DISK_SEEKSET,
	DISK_SEEKCUR,
	DISK_SEEKEND,
	DISK_FINISH
};

struct blk_info {
	uint16_t num_cylinders;
	uint16_t num_heads;
	uint16_t sectors_track;
	uint32_t num_sectors;		// mandatory
	uint16_t bytes_sector;		// mandatory
	uint16_t media_desc;
};
