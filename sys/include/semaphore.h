/**
 * @brief Semaphore data structure.
 */
struct sem {
	struct queue *sem_queue;			/*!< queue for tasks waiting on the semaphore */
	int32_t count;					/*!< semaphore counter */
};

typedef volatile struct sem sem_t;

int32_t hf_seminit(sem_t *s, int32_t value);
int32_t hf_semdestroy(sem_t *s);
void hf_semwait(sem_t *s);
void hf_sempost(sem_t *s);
