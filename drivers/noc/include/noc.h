/* communication errors */
#define HF_INVALID_CPU		-201
#define HF_IF_NOT_READY		-202
#define HF_COMM_TIMEOUT		-203
#define HF_SEQ_ERROR		-204
#define HF_COMM_BUSY		-205
#define HF_COMM_UNFEASIBLE	-206
#define HF_COMM_ERROR		-207

/* packet header offsets */
#define PKT_HEADER_SIZE		8
#define PKT_TARGET_CPU		0
#define PKT_PAYLOAD		1
#define PKT_SOURCE_CPU		2
#define PKT_SOURCE_TASK		3
#define PKT_TARGET_TASK		4
#define PKT_MSG_SIZE		5
#define PKT_SEQ			6
#define PKT_CHANNEL		7

#define NOC_COLUMN(core_n)	((core_n) % NOC_WIDTH)
#define NOC_LINE(core_n)	((core_n) / NOC_WIDTH)

/* array of queues - each task can have its own custom queue */
struct queue *pktdrv_tqueue[MAX_TASKS];
/* queue of free packets - number of packets is NOC_PACKET_SLOTS */
struct queue *pktdrv_queue;

void ni_init(void);
void recv_isr(void *arg);
void sysmon(void);

int32_t hf_comm_create(uint16_t id, uint16_t packets);
int32_t hf_comm_destroy(uint16_t id);
int32_t hf_recv(uint16_t *source_cpu, uint16_t *source_id, int8_t *buf, uint16_t *size, uint16_t channel);
int32_t hf_send(uint16_t target_cpu, uint16_t target_id, int8_t *buf, uint16_t size, uint16_t channel);
int32_t hf_queryid(uint16_t target_cpu, int8_t *desc);
int32_t hf_getid(uint16_t *cpu, int32_t *tid);
