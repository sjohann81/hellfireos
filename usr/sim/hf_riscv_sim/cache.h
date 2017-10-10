struct cache_line_s {
	uint8_t valid;
	uint8_t dirty;
	uint32_t tag;
	uint32_t *block;
};

struct miss_penalty_s {
	uint32_t setup_time;
	uint32_t burst_time;
};

struct access_s {
	uint64_t hits;
	uint64_t misses;
	uint64_t reads;
	uint64_t writes;
};

struct config_masks_s {
	uint32_t tag_mask;
	uint32_t tag_shift;
	uint32_t line_mask;
	uint32_t line_shift;
	uint32_t block_mask;
};

struct cache_s {
	uint8_t level;
	int8_t *policy;
	int32_t block_size;
	int32_t num_lines;
	struct miss_penalty_s miss_penalty;
	struct config_masks_s config;
	struct access_s access;
	struct cache_line_s *cache_lines;
	struct cache_s *next_level;
};

int32_t cache_init(struct cache_s *cache, int8_t *policy, uint32_t block_size, uint32_t num_lines, uint32_t miss_setup, uint32_t miss_burst);
uint32_t cache_read(struct cache_s *cache, uint32_t address);
uint32_t cache_write(struct cache_s *cache, uint32_t address);
void cache_finish(struct cache_s *cache);
