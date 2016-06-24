#if MUTEX_TYPE == 0
/**
 * @brief Mutex data structure.
 */
struct mtx {
	int32_t lock;					/*!< mutex lock, atomically modified */
};

typedef volatile struct mtx mutex_t;
#endif

#if MUTEX_TYPE == 1
struct mtx {
	uint8_t level[MAX_TASKS];
	uint8_t waiting[MAX_TASKS - 1];
};

typedef volatile struct mtx mutex_t;
#endif

void hf_mtxinit(mutex_t *m);
void hf_mtxlock(mutex_t *m);
void hf_mtxunlock(mutex_t *m);
