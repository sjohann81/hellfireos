struct fs_ecodes {
	int32_t code;
	int8_t *description;
} fs_errorcodes[]  = {
	{UHFS_EOK,		"no error"},
	{UHFS_EERROR,		"generic error"},
	{UHFS_ENOTMOUNTED,	"filesystem not mounted"},
	{UHFS_EALREADYMOUNTED,	"filesystem already mounted"},
	{UHFS_EINVSECTORSIZE,	"invalid media sector size"},
	{UHFS_EINVBLOCKSIZE,	"invalid media block size"},
	{UHFS_ESTORAGEFULL,	"storage is full"},
	{UHFS_EPATHNOTFOUND,	"path not found"},
	{UHFS_ENOTADIR,		"not a directory"},
	{UHFS_ENOTAFILE,	"not a file"},
	{UHFS_EFILEEXISTS,	"file already exists"},
	{UHFS_EFILENOTOPEN,	"file not open"},
	{UHFS_EDIRNOTEMPTY,	"directory is not empty"}
};
