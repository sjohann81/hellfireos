/**
 * @brief Queue data structure.
 */
struct queue {
	int32_t size;					/*!< queue size (maximum number of elements) */
	int32_t elem;					/*!< number of elements queued */
	int32_t head;					/*!< first element of the queue */
	int32_t tail;					/*!< last element of the queue */
	void **data;					/*!< pointer to an array of pointers to node data */
};

struct queue *hf_queue_create(int32_t size);
int32_t hf_queue_destroy(struct queue *q);
int32_t hf_queue_count(struct queue *q);
int32_t hf_queue_addtail(struct queue *q, void *ptr);
void *hf_queue_remhead(struct queue *q);
void *hf_queue_remtail(struct queue *q);
void *hf_queue_get(struct queue *q, int32_t elem);
int32_t hf_queue_set(struct queue *q, int32_t elem, void *ptr);
int32_t hf_queue_swap(struct queue *q, int32_t elem1, int32_t elem2);
