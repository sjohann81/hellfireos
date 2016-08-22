/**
 * @file condvar.c
 * @author Sergio Johann Filho
 * @date May 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * Condition variable synchronization primitives.
 * 
 */

#include <hal.h>
#include <libc.h>
#include <queue.h>
#include <mutex.h>
#include <condvar.h>
#include <kernel.h>
#include <panic.h>
#include <task.h>
#include <ecodes.h>

/**
 * @brief Initializes a condition variable
 * 
 * @param c is a pointer to a condition variable.
 *
 * @return ERR_OK on success and ERR_ERROR if the condition variable could not be allocated in memory.
 */
int32_t hf_condinit(cond_t *c)
{
	volatile uint32_t status;

	status = _di();
	c->cond_queue = hf_queue_create(MAX_TASKS);
	if (c->cond_queue == NULL){
		_ei(status);
		return ERR_ERROR;
	}else{
		_ei(status);
		return ERR_OK;
	}
}

/**
 * @brief Destroys a condition variable.
 * 
 * @param c is a pointer to a condition variable.
 * 
 * @return ERR_OK on success and ERR_ERROR if the condition variable could not be
 * removed from memory.
 */
int32_t hf_conddestroy(cond_t *c)
{
	volatile uint32_t status;

	status = _di();
	if (hf_queue_destroy(c->cond_queue)){
		_ei(status);
		return ERR_ERROR;
	}else{
		_ei(status);
		return ERR_OK;
	}
}

/**
 * @brief Wait on a condition variable.
 * 
 * @param c is a pointer to a condition variable.
 * @param m is a pointer to a mutex.
 * 
 * Implements the atomic condition wait operation. The call should always be invoked
 * with the mutex locked. The current task is put in a queue on the condition variable,
 * its state is set to blocked and unlocks the mutex atomically, then yields the
 * processor. When woke up (by a signalling task), the task locks the mutex and returns.
 */
void hf_condwait(cond_t *c, mutex_t *m)
{
	volatile uint32_t status;
	struct tcb_entry *krnl_task2;

	status = _di();
	krnl_task2 = &krnl_tcb[krnl_current_task];
	if (hf_queue_addtail(c->cond_queue, krnl_task2))
		panic(PANIC_NUTS_SEM);
	else
		krnl_task2->state = TASK_BLOCKED;
	hf_mtxunlock(m);
	_ei(status);
	hf_yield();
	hf_mtxlock(m);
}

/**
 * @brief Signal a condition variable.
 * 
 * @param c is a pointer to a condition variable.
 * 
 * Implements the condition signal operation for one waiting task. The call removes a
 * task from the waiting queue and unblocks it. If no tasks are waiting for the condition,
 * the signal is lost.
 */
void hf_condsignal(cond_t *c)
{
	volatile uint32_t status;
	struct tcb_entry *krnl_task2;

	status = _di();
	krnl_task2 = hf_queue_remhead(c->cond_queue);
	if (krnl_task2)
		krnl_task2->state = TASK_READY;
	_ei(status);
}

/**
 * @brief Signal (broadcast) a condition variable.
 * 
 * @param c is a pointer to a condition variable.
 * 
 * Implements the condition signal broadcast operation for all waiting tasks. The call
 * unblocks and removes all tasks from the waiting queue. If no tasks are waiting for the
 * condition, the signal is lost.
 */
void hf_condbroadcast(cond_t *c)
{
	volatile uint32_t status;
	struct tcb_entry *krnl_task2;
		
	status = _di();
	while (hf_queue_count(c->cond_queue)){
		krnl_task2 = hf_queue_remhead(c->cond_queue);
		if (krnl_task2)
			krnl_task2->state = TASK_READY;
	}
	_ei(status);
}
