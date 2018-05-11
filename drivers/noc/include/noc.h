/**
 * @file noc.h
 * @author Sergio Johann Filho
 * @date April 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * Network-on-Chip driver error codes and packet header offsets.
 */

#define ERR_INVALID_CPU		-201		/*!< invalid cpu */
#define ERR_IF_NOT_READY	-202		/*!< network interface is not ready */
#define ERR_COMM_TIMEOUT	-203		/*!< a pending communication has timed out */
#define ERR_SEQ_ERROR		-204		/*!< packet sequence mismatch */
#define ERR_COMM_BUSY		-205		/*!< communication channel is busy */
#define ERR_COMM_UNFEASIBLE	-206		/*!< communication is not feasible */
#define ERR_COMM_ERROR		-207		/*!< general communication error */
#define ERR_COMM_EMPTY		-208		/*!< reception queue is empty */

#define PKT_HEADER_SIZE		8
#define PKT_TARGET_CPU		0
#define PKT_PAYLOAD		1
#define PKT_SOURCE_CPU		2
#define PKT_SOURCE_PORT		3
#define PKT_TARGET_PORT		4
#define PKT_MSG_SIZE		5
#define PKT_SEQ			6
#define PKT_CHANNEL		7

#define NOC_COLUMN(core_n)	((core_n) % NOC_WIDTH)
#define NOC_LINE(core_n)	((core_n) / NOC_WIDTH)

/**
 * @brief Array of associations between tasks and reception ports.
 */
uint16_t pktdrv_ports[MAX_TASKS];

/**
 * @brief Array of queues. Each task can have its own custom sized queue.
 */
struct queue *pktdrv_tqueue[MAX_TASKS];

/**
 * @brief Queue of free (shared) packets. The number of packets is NOC_PACKET_SLOTS.
 */
struct queue *pktdrv_queue;

void ni_init(void);
void ni_isr(void *arg);

uint16_t hf_cpuid(void);
uint16_t hf_ncores(void);
int32_t hf_comm_create(uint16_t id, uint16_t port, uint16_t packets);
int32_t hf_comm_destroy(uint16_t id);
int32_t hf_recvprobe(void);
int32_t hf_recv(uint16_t *source_cpu, uint16_t *source_port, int8_t *buf, uint16_t *size, uint16_t channel);
int32_t hf_send(uint16_t target_cpu, uint16_t target_port, int8_t *buf, uint16_t size, uint16_t channel);
int32_t hf_recvack(uint16_t *source_cpu, uint16_t *source_port, int8_t *buf, uint16_t *size, uint16_t channel);
int32_t hf_sendack(uint16_t target_cpu, uint16_t target_port, int8_t *buf, uint16_t size, uint16_t channel, uint32_t timeout);
// hf_request(), hf_reply()
