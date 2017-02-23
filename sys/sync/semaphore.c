/**
 * @file semaphore.c
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
 * Semaphore synchronization primitives.
 * 
 */

#include <hal.h>
#include <libc.h>
#include <queue.h>
#include <semaphore.h>
#include <kernel.h>
#include <panic.h>
#include <task.h>
#include <ecodes.h>

/**
 * @brief Initializes a semaphore and defines its initial value.
 * 
 * @param s is a pointer to a semaphore.
 * @param value is the semaphore initial value.
 * 
 * @return ERR_OK on success and ERR_ERROR if the semaphore could not be allocated in
 * memory or its initial value is less than zero.
 */
int32_t hf_seminit(sem_t *s, int32_t value)
{
	volatile uint32_t status;

	status = _di();
	s->sem_queue = hf_queue_create(MAX_TASKS);
	if ((s->sem_queue == NULL) || (value < 0)){
		_ei(status);
		return ERR_ERROR;
	}else{
		s->count = value;
		_ei(status);
		return ERR_OK;
	}
}

/**
 * @brief Destroys a semaphore.
 * 
 * @param s is a pointer to a semaphore.
 * 
 * @return ERR_OK on success and ERR_ERROR if the semaphore could not be removed from memory.
 */
int32_t hf_semdestroy(sem_t *s)
{
	volatile uint32_t status;

	status = _di();
	if (hf_queue_destroy(s->sem_queue)){
		_ei(status);
		return ERR_ERROR;
	}else{
		_ei(status);
		return ERR_OK;
	}
}

/**
 * @brief Wait on a semaphore.
 * 
 * @param s is a pointer to a semaphore.
 * 
 * Implements the atomic P() operation. The semaphore count is decremented and
 * calling task is blocked and queued on the semaphore if the count reaches a negative
 * value. If not, the task continues its execution.
 */
void hf_semwait(sem_t *s)
{
	volatile uint32_t status;
	struct tcb_entry *krnl_task2;
	
	status = _di();
	s->count--;
	if (s->count < 0){
		krnl_task2 = &krnl_tcb[krnl_current_task];
		if (hf_queue_addtail(s->sem_queue, krnl_task2))
			panic(PANIC_NUTS_SEM);
		else
			krnl_task2->state = TASK_BLOCKED;
		_ei(status);
		hf_yield();
	}else{
		_ei(status);
	}
}

/**
 * @brief Signal a semaphore.
 * 
 * @param s is a pointer to a semaphore.
 * 
 * Implements the atomic V() operation. The semaphore count is incremented and
 * the task from the head of the semaphore queue is unblocked if the count is less
 * than or equal to zero.
 */
void hf_sempost(sem_t *s)
{
	volatile uint32_t status;
	struct tcb_entry *krnl_task2;

	status = _di();
	s->count++;
	if (s->count <= 0){
		krnl_task2 = hf_queue_remhead(s->sem_queue);
		if (krnl_task2 == NULL)
			panic(PANIC_NUTS_SEM);
		else
			krnl_task2->state = TASK_READY;
	}
	_ei(status);
}
