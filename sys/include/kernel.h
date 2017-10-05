/**
 * @file kernel.h
 * @author Sergio Johann Filho
 * @date February 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * Kernel data structures.
 * 
 */

/* task status definitions */
#define TASK_IDLE			0		/*!< task does not exist / not ready */
#define TASK_READY			1		/*!< task ready to run (on run queue) */
#define TASK_RUNNING			2		/*!< task running (only one task/core can be in this state, on run queue) */
#define TASK_BLOCKED			3		/*!< task blocked, can be resumed later (on run queue) */
#define TASK_DELAYED			4		/*!< task being delayed (on delay queue) */
#define TASK_WAITING			5		/*!< task waiting for an event (on event queue) */

/**
 * @brief Task control block (TCB) and processor control block (PCB) entry data structures.
 */
struct tcb_entry {
	uint16_t id;					/*!< task id */
	int8_t name[20];				/*!< task description (or name) */
	uint8_t state;					/*!< 0 - idle,  1 - ready,  2 - running, 3 - blocked, 4 - delayed, 5 - waiting */
	uint8_t priority;				/*!< [1 .. 29] - critical, [30 .. 99] - system, [100 .. 255] - application */
	uint8_t priority_rem;				/*!< remaining priority */
	uint8_t critical;				/*!< critical event, interrupt request */
	uint32_t delay;					/*!< delay to enter in the run/RT queue */
	uint32_t rtjobs;				/*!< total RT task jobs executed */
	uint32_t bgjobs;				/*!< total BE task jobs executed */
	uint32_t deadline_misses;			/*!< task realtime deadline misses */
	uint16_t period;				/*!< task period */
	uint16_t capacity;				/*!< task capacity */
	uint16_t deadline;				/*!< task deadline */
	uint16_t capacity_rem;				/*!< remaining capacity on period */
	uint16_t deadline_rem;				/*!< remaining time slices on period */
	context task_context;				/*!< task context */
	void (*ptask)(void);				/*!< task entry point, pointer to function */
	size_t *pstack;					/*!< task stack area (bottom) */
	uint32_t stack_size;				/*!< task stack size */
	void *other_data;				/*!< pointer to other data related to this task */
};

struct pcb_entry {
	int32_t (*sched_rt)();				/*!< pointer to the realtime scheduler */
	int32_t (*sched_be)();				/*!< pointer to the best effort scheduler */
	uint32_t coop_cswitch;				/*!< cooperative context switches */
	uint32_t preempt_cswitch;			/*!< preeptive context switches */
	uint32_t interrupts;				/*!< number of non-masked interrupts */
	uint32_t tick_time;				/*!< tick time in microsseconds */
	/* much more stuff should be here! */
};

/**
 * @brief The task control block and processor control block
 */
struct tcb_entry krnl_tcb[MAX_TASKS];
struct pcb_entry krnl_pcb;

struct tcb_entry *krnl_task;				/*!< pointer to a task control block entry */
uint16_t krnl_tasks;					/*!< number of tasks in the system */
uint16_t krnl_current_task;				/*!< the current running task id */
uint16_t krnl_schedule;					/*!< scheduler enable / disable flag */
struct queue *krnl_run_queue;				/*!< pointer to a queue of best effort tasks */
struct queue *krnl_delay_queue;				/*!< pointer to a queue of delayed tasks */
struct queue *krnl_rt_queue;				/*!< pointer to a queue of real time tasks */
struct queue *krnl_event_queue;				/*!< pointer to a queue of tasks waiting for an event */
uint8_t krnl_heap[HEAP_SIZE];				/*!< contiguous heap memory area to be used as a memory pool. the memory allocator (malloc() and free()) controls this data structure */
uint32_t krnl_free;					/*!< amount of free heap memory, in bytes */
