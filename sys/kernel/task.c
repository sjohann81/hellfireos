/**
 * @file task.c
 * @author Sergio Johann Filho
 * @date March 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * Task management primitives and auxiliary functions.
 * 
 */

#include <hal.h>
#include <libc.h>
#include <kprintf.h>
#include <malloc.h>
#include <queue.h>
#include <kernel.h>
#include <panic.h>
#include <scheduler.h>
#include <task.h>
#include <ecodes.h>

/**
 * @brief Get a task id by its name.
 * 
 * @param name is a pointer to an array holding the task name.
 * 
 * @return task id if the task is found and ERR_INVALID_NAME otherwise.
 */
int32_t hf_id(int8_t *name)
{
	int32_t i;

#if KERNEL_LOG == 2
	dprintf("hf_id() %d ", (uint32_t)_read_us());
#endif
	for (i = 0; i < MAX_TASKS; i++){
		if (krnl_tcb[i].ptask){
			if (strcmp(krnl_tcb[i].name, name) == 0)
				return krnl_tcb[i].id;
		}
	}
	return ERR_INVALID_NAME;
}

/**
 * @brief Get a task name by its id.
 * 
 * @param id is a task id number.
 * 
 * @return task name if the task is found and NULL otherwise.
 */
int8_t *hf_name(uint16_t id)
{
#if KERNEL_LOG == 2
	dprintf("hf_name() %d ", (uint32_t)_read_us());
#endif
	if (id < MAX_TASKS)
		if (krnl_tcb[id].ptask)
			return krnl_tcb[id].name;
	return NULL;
}

/**
 * @brief Get the current task id.
 * 
 * @return current task id.
 */
uint16_t hf_selfid(void)
{
#if KERNEL_LOG == 2
	dprintf("hf_selfid() %d ", (uint32_t)_read_us());
#endif
	return krnl_task->id;
}

/**
 * @brief Get the current task name.
 * 
 * @return current task name.
 */
int8_t *hf_selfname(void)
{
#if KERNEL_LOG == 2
	dprintf("hf_selfname() %d ", (uint32_t)_read_us());
#endif
	return krnl_task->name;
}

/**
 * @brief Get the current state of a task.
 * 
 * @param id is a task id number.
 * 
 * @return task state the task if found (TASK_IDLE, TASK_READY, TASK_RUNNING, TASK_BLOCKED, TASK_DELAYED or TASK_WAITING) and ERR_INVALID_ID otherwise.
 */
int32_t hf_state(uint16_t id)
{
#if KERNEL_LOG == 2
	dprintf("hf_state() %d ", (uint32_t)_read_us());
#endif
	if (id < MAX_TASKS)
		if (krnl_tcb[id].ptask)
			return krnl_tcb[id].state;
	return ERR_INVALID_ID;
}

/**
 * @brief Get the number of executed jobs of a task.
 * 
 * @param id is a task id number.
 * 
 * @return jobs executed by the task if found and ERR_INVALID_ID otherwise.
 */
int32_t hf_jobs(uint16_t id)
{
#if KERNEL_LOG == 2
	dprintf("hf_jobs() %d ", (uint32_t)_read_us());
#endif
	if (id < MAX_TASKS)
		if (krnl_tcb[id].ptask){
			if (krnl_tcb[id].period)
				return krnl_tcb[id].rtjobs;
			else
				return krnl_tcb[id].bgjobs;
		}
	return ERR_INVALID_ID;
}

/**
 * @brief Get the number of deadline misses of a task.
 * 
 * @param id is a task id number.
 * 
 * @return deadlines missed by the task if found and ERR_INVALID_ID otherwise.
 */
int32_t hf_dlm(uint16_t id){
#if KERNEL_LOG == 2
	dprintf("hf_dlm() %d ", (uint32_t)_read_us());
#endif
	if (id < MAX_TASKS)
		if (krnl_tcb[id].ptask)
			return krnl_tcb[id].deadline_misses;
	return ERR_INVALID_ID;
}

/**
 * @brief Set the priority of a best effort task.
 * 
 * @param id is a task id number.
 * @param priority is the task priority ([1 .. 29] - critical, [30 .. 99] - system, [100 .. 255] - application)
 * 
 * @return ERR_OK if the task exists and is a best effort one or ERR_INVALID_ID otherwise.
 */
int32_t hf_priorityset(uint16_t id, uint8_t priority)
{
	struct tcb_entry *krnl_task2;
	
#if KERNEL_LOG == 2
	dprintf("hf_priorityset() %d ", (uint32_t)_read_us());
#endif
	if (id < MAX_TASKS){
		krnl_task2 = &krnl_tcb[id];
		if (krnl_task2->ptask){
			if (krnl_task2->period == 0){
				krnl_task2->priority = priority;
				krnl_task2->priority_rem = priority;
				
				return ERR_OK;
			}
		}
	}
	return ERR_INVALID_ID;
}

/**
 * @brief Get the priority of a best effort task.
 * 
 * @param id is a task id number.
 * 
 * @return task priority if the task exists and is a best effort one or ERR_INVALID_ID otherwise.
 */
int32_t hf_priorityget(uint16_t id)
{
	struct tcb_entry *krnl_task2;
	
#if KERNEL_LOG == 2
	dprintf("hf_priorityget() %d ", (uint32_t)_read_us());
#endif
	if (id < MAX_TASKS){
		krnl_task2 = &krnl_tcb[id];
		if (krnl_task2->ptask){
			if (krnl_task2->period == 0)
				return krnl_task2->priority;
		}
	}
	return ERR_INVALID_ID;
}

/**
 * @brief Spawn a new task.
 * 
 * @param task is a pointer to a task function / body.
 * @param period is the task RT period (in quantum / tick units).
 * @param capacity is the amount of work to be executed in a period (in quantum / tick units).
 * @param deadline is the task deadline to complete the work in the period (in quantum / tick units).
 * @param name is a string used to identify a task.
 * @param stack_size is the stack memory to be allocated for the task.
 * 
 * @return task id if the task is created, ERR_EXCEED_MAX_NUM if the maximum number of tasks in the system
 * is exceeded, ERR_INVALID_PARAMETER if impossible RT parameters are specified or ERR_OUT_OF_MEMORY if
 * the system fails to allocate memory for the task resources.
 * 
 * If a task has defined realtime parameters, it is put on the RT queue, if not
 * (period 0, capacity 0 and deadline 0), it is put on the BE queue.
 * WARNING: Task stack size should be always configured correctly, considering data
 * declared on the auto region (local variables) and around 1024 of spare memory for the OS.
 * For example, if you declare a buffer of 5000 bytes, stack size should be at least 6000.
 */
int32_t hf_spawn(void (*task)(), uint16_t period, uint16_t capacity, uint16_t deadline, int8_t *name, uint32_t stack_size)
{
	volatile uint32_t status, i = 0;

#if KERNEL_LOG == 2
	dprintf("hf_spawn() %d ", (uint32_t)_read_us());
#endif
	if ((period < capacity) || (deadline < capacity))
		return ERR_INVALID_PARAMETER;
	
	status = _di();
	while ((krnl_tcb[i].ptask != 0) && (i < MAX_TASKS))
		i++;
	if (i == MAX_TASKS){
		kprintf("\nKERNEL: task not added - MAX_TASKS: %d", MAX_TASKS);
		_ei(status);
		return ERR_EXCEED_MAX_NUM;
	}
	krnl_tasks++;
	krnl_task = &krnl_tcb[i];
	krnl_task->id = i;
	strncpy(krnl_task->name, name, sizeof(krnl_task->name));
	krnl_task->state = TASK_IDLE;
	krnl_task->priority = 100;
	krnl_task->priority_rem = 100;
	krnl_task->delay = 0;
	krnl_task->period = period;
	krnl_task->capacity = capacity;
	krnl_task->deadline = deadline;
	krnl_task->capacity_rem = capacity;
	krnl_task->deadline_rem = deadline;
	krnl_task->rtjobs = 0;
	krnl_task->bgjobs = 0;
	krnl_task->deadline_misses = 0;
	krnl_task->ptask = task;
	stack_size += 3;
	stack_size >>= 2;
	stack_size <<= 2;
	krnl_task->stack_size = stack_size;
	krnl_task->pstack = (size_t *)hf_malloc(stack_size);
	_set_task_sp(krnl_task->id, (size_t)krnl_task->pstack + (stack_size - 4));
	_set_task_tp(krnl_task->id, krnl_task->ptask);
	if (krnl_task->pstack){
		krnl_task->pstack[0] = STACK_MAGIC;
		kprintf("\nKERNEL: [%s], id: %d, p:%d, c:%d, d:%d, addr: %x, sp: %x, ss: %d bytes", krnl_task->name, krnl_task->id, krnl_task->period, krnl_task->capacity, krnl_task->deadline, krnl_task->ptask, _get_task_sp(krnl_task->id), stack_size);
		if (period){
			if (hf_queue_addtail(krnl_rt_queue, krnl_task)) panic(PANIC_CANT_PLACE_RT);
		}else{
			if (hf_queue_addtail(krnl_run_queue, krnl_task)) panic(PANIC_CANT_PLACE_RUN);
		}
	}else{
		krnl_task->ptask = 0;
		krnl_tasks--;
		kprintf("\nKERNEL: task not added (out of memory)");
		i = ERR_OUT_OF_MEMORY;
	}
	krnl_task = &krnl_tcb[krnl_current_task];
	_ei(status);
	
	return i;
}

/**
 * @brief Yields the current task.
 * 
 * The current task gives up execution and the best effort scheduler is invoked.
 */
void hf_yield(void)
{
	int32_t rc;
	volatile int32_t status;

	status = _di();
#if KERNEL_LOG >= 1
		dprintf("hf_yield() %d ", (uint32_t)_read_us());
#endif	
	krnl_task = &krnl_tcb[krnl_current_task];
	rc = _context_save(krnl_task->task_context);
	if (rc){
		_ei(status);
		return;
	}
	if (krnl_task->state == TASK_RUNNING)
		krnl_task->state = TASK_READY;
	if (krnl_task->pstack[0] != STACK_MAGIC)
		panic(PANIC_STACK_OVERFLOW);
	if (krnl_tasks > 0){
		krnl_current_task = krnl_pcb.sched_be();
		krnl_task->state = TASK_RUNNING;
		krnl_pcb.coop_cswitch++;
#if KERNEL_LOG >= 1
		dprintf("\n%d %d %d %d %d ", krnl_current_task, krnl_task->period, krnl_task->capacity, krnl_task->deadline, (uint32_t)_read_us());
#endif
		_context_restore(krnl_task->task_context, 1);
		panic(PANIC_UNKNOWN);
	}else{
		panic(PANIC_NO_TASKS_LEFT);
	}
}

/**
 * @brief Blocks a task.
 * 
 * @param id is a task id number.
 * 
 * @return ERR_OK on success, ERR_INVALID_ID if the referenced task does not exist or ERR_ERROR if the task is already in the blocked state.
 * 
 * The task is marked as TASK_BLOCKED so the scheduler doesn't select it as a candidate for scheduling.
 * The blocking state is acomplished without removing the task from the run queue, reducing the cost of
 * the operation in cases where the task state is switched frequently (such as in semaphore primitives).
 */
int32_t hf_block(uint16_t id)
{
	volatile uint32_t status;

#if KERNEL_LOG == 2
	dprintf("hf_block() %d ", (uint32_t)_read_us());
#endif
	status = _di();
	if (id == 0){
		kprintf("\nKERNEL: can't block the idle task");
		_ei(status);
		return ERR_INVALID_ID;
	}
	krnl_task = &krnl_tcb[id];
	if ((krnl_task->ptask == 0) || (id >= MAX_TASKS)){
		kprintf("\nKERNEL: task doesn't exist");
		krnl_task = &krnl_tcb[krnl_current_task];
		_ei(status);
		return ERR_INVALID_ID;
	}
	if (krnl_task->state == TASK_BLOCKED){
		kprintf("\nKERNEL: can't block an already blocked task");
		krnl_task = &krnl_tcb[krnl_current_task];
		_ei(status);
		return ERR_ERROR;
	}
	krnl_task->state = TASK_BLOCKED;
	krnl_task = &krnl_tcb[krnl_current_task];
	_ei(status);
	
	return ERR_OK;
}

/**
 * @brief Resumes a blocked task.
 * 
 * @param id is a task id number.
 * 
 * @return ERR_OK on success, ERR_INVALID_ID if the referenced task does not exist or ERR_ERROR if the task is not in the blocked state.
 * 
 * The task must be in the TASK_BLOCKED state in order to be resumed. 
 * The task is marked as TASK_BLOCKED so the scheduler doesn't select it as a candidate for scheduling.
 * The blocking state is acomplished without removing the task from the run queue, reducing the cost of
 * the operation in cases where the task state is switched frequently (such as in semaphore primitives).
 */
int32_t hf_resume(uint16_t id)
{
	volatile uint32_t status;

#if KERNEL_LOG == 2
	dprintf("hf_resume() %d ", (uint32_t)_read_us());
#endif
	status = _di();
	if (id == 0){
		kprintf("\nKERNEL: can't resume the idle task");
		_ei(status);
		return ERR_INVALID_ID;
	}
	krnl_task = &krnl_tcb[id];
	if ((krnl_task->ptask == 0) || (id >= MAX_TASKS)){
		kprintf("\nKERNEL: task doesn't exist");
		krnl_task = &krnl_tcb[krnl_current_task];
		_ei(status);
		return ERR_INVALID_ID;
	}
	if (krnl_task->state != TASK_BLOCKED){
		kprintf("\nKERNEL: can't resume a non blocked task");
		krnl_task = &krnl_tcb[krnl_current_task];
		_ei(status);
		return ERR_ERROR;
	}
	krnl_task->state = TASK_READY;
	krnl_task = &krnl_tcb[krnl_current_task];
	_ei(status);

	return ERR_OK;
}

/**
 * @brief Kills a task.
 * 
 * @param id is a task id number.
 * 
 * @return ERR_OK on success or ERR_INVALID_ID if the referenced task does not exist.
 * 
 * All memory allocated during the task initialization is freed, the TCB entry is cleared and
 * the task is removed from its run queue.
 */
int32_t hf_kill(uint16_t id)
{
	volatile uint32_t status;
	int32_t i, j, k;
	struct tcb_entry *krnl_task2;

#if KERNEL_LOG == 2
	dprintf("hf_kill() %d ", (uint32_t)_read_us());
#endif
	status = _di();
	if (id == 0){
		kprintf("\nKERNEL: can't kill the idle task");
		_ei(status);
		return ERR_INVALID_ID;
	}
	krnl_task = &krnl_tcb[id];
	if ((krnl_task->ptask == 0) || (id >= MAX_TASKS)){
		kprintf("\nKERNEL: task doesn't exist");
		krnl_task = &krnl_tcb[krnl_current_task];
		_ei(status);
		return ERR_INVALID_ID;
	}

	krnl_task->id = -1;
	krnl_task->ptask = 0;
	hf_free(krnl_task->pstack);
	_set_task_sp(krnl_task->id, 0);
	_set_task_tp(krnl_task->id, 0);
	krnl_task->state = TASK_IDLE;
	krnl_tasks--;

	if (krnl_task->period){
		k = hf_queue_count(krnl_rt_queue);
		for (i = 0; i < k; i++)
			if (hf_queue_get(krnl_rt_queue, i) == krnl_task) break;
		if (!k || i == k) panic(PANIC_NO_TASKS_RT);
		for (j = i; j > 0; j--)
			if (hf_queue_swap(krnl_rt_queue, j, j-1)) panic(PANIC_CANT_SWAP);
		krnl_task2 = hf_queue_remhead(krnl_rt_queue);
	}else{
		k = hf_queue_count(krnl_run_queue);
		for (i = 0; i < k; i++)
			if (hf_queue_get(krnl_run_queue, i) == krnl_task) break;
		if (!k || i == k) panic(PANIC_NO_TASKS_RUN);
		for (j = i; j > 0; j--)
			if (hf_queue_swap(krnl_run_queue, j, j-1)) panic(PANIC_CANT_SWAP);
		krnl_task2 = hf_queue_remhead(krnl_run_queue);
	}
	if (!krnl_task2 || krnl_task2 != krnl_task) panic(PANIC_UNKNOWN_TASK_STATE);
	
	krnl_task = &krnl_tcb[krnl_current_task];
	kprintf("\nKERNEL: task died, id: %d, tasks left: %d", id, krnl_tasks);
	if (hf_selfid() == id){
		_ei(status);
		hf_yield();
	}else{
		_ei(status);
	}

	return ERR_OK;
}

/**
 * @brief Delays a task for an amount of time.
 * 
 * @param id is a task id number.
 * @param delay is the amount of time (in quantum / tick units).
 * 
 * @return ERR_OK on success or ERR_INVALID_ID if the referenced task does not exist.
 * 
 * A task is removed from its run queue and its state is marked as TASK_DELAYED. The task is put on the delay queue
 * and remains there until the dispatcher places it back to its run queue. Time is managed by the task dispatcher, which
 * counts down delays, controls the delay queue by cycling the tasks and removing them when the task delay has passed.
 */
int32_t hf_delay(uint16_t id, uint32_t delay)
{
	int32_t i, j, k;
	volatile uint32_t status;
	struct tcb_entry *krnl_task2;

#if KERNEL_LOG == 2
	dprintf("hf_delay() %d ", (uint32_t)_read_us());
#endif
	if (delay == 0) return ERR_ERROR;
	
	status = _di();
	if (id == 0){
		kprintf("\nKERNEL: can't delay the idle task");
		_ei(status);
		return ERR_INVALID_ID;
	}
	krnl_task = &krnl_tcb[id];
	if ((krnl_task->ptask == 0) || (id >= MAX_TASKS)){
		kprintf("\nKERNEL: task doesn't exist");
		krnl_task = &krnl_tcb[krnl_current_task];
		_ei(status);
		return ERR_INVALID_ID;
	}
	if (krnl_task->period){
		k = hf_queue_count(krnl_rt_queue);
		for (i = 0; i < k; i++)
			if (hf_queue_get(krnl_rt_queue, i) == krnl_task) break;
		if (!k || i == k) panic(PANIC_NO_TASKS_RT);
		for (j = i; j > 0; j--)
			if (hf_queue_swap(krnl_rt_queue, j, j-1)) panic(PANIC_CANT_SWAP);
		krnl_task2 = hf_queue_remhead(krnl_rt_queue);
	}else{
		k = hf_queue_count(krnl_run_queue);
		for (i = 0; i < k; i++)
			if (hf_queue_get(krnl_run_queue, i) == krnl_task) break;
		if (!k || i == k) panic(PANIC_NO_TASKS_RUN);
		for (j = i; j > 0; j--)
			if (hf_queue_swap(krnl_run_queue, j, j-1)) panic(PANIC_CANT_SWAP);
		krnl_task2 = hf_queue_remhead(krnl_run_queue);
	}
	
	krnl_task->state = TASK_DELAYED;
	krnl_task->delay = delay;
	if (hf_queue_addtail(krnl_delay_queue, krnl_task2)) panic(PANIC_CANT_PLACE_DELAY);
	krnl_task = &krnl_tcb[krnl_current_task];
	_ei(status);
	
	return ERR_OK;
}
