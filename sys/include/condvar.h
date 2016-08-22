/**
 * @brief Condition variable data structure.
 */
struct condvar {
	struct queue *cond_queue;			/*!< queue for tasks waiting on the condition variable */
	mutex_t	mutex;					/*!< mutex used for the critical section associated with the condition variable */
};

typedef volatile struct condvar cond_t;

int32_t hf_condinit(cond_t *c);
int32_t hf_conddestroy(cond_t *c);
void hf_condwait(cond_t *c, mutex_t *m);
void hf_condsignal(cond_t *c);
void hf_condbroadcast(cond_t *c);
