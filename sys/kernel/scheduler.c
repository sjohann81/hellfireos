/**
 * @file scheduler.c
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
 * Kernel two-level scheduler and task queue management.
 * 
 */

#include <hal.h>
#include <libc.h>
#include <kprintf.h>
#include <queue.h>
#include <kernel.h>
#include <panic.h>
#include <scheduler.h>

static void process_delay_queue(void)
{
	int32_t i, k;
	struct tcb_entry *krnl_task2;

	k = hf_queue_count(krnl_delay_queue);
	for (i = 0; i < k; i++){
		krnl_task2 = hf_queue_remhead(krnl_delay_queue);
		if (!krnl_task2) panic(PANIC_NO_TASKS_DELAY);
		if (--krnl_task2->delay == 0){
			if (krnl_task2->period){
				if (hf_queue_addtail(krnl_rt_queue, krnl_task2)) panic(PANIC_CANT_PLACE_RT);
			}else{
				if (hf_queue_addtail(krnl_run_queue, krnl_task2)) panic(PANIC_CANT_PLACE_RUN);
			}
		}else{
			if (hf_queue_addtail(krnl_delay_queue, krnl_task2)) panic(PANIC_CANT_PLACE_DELAY);
		}
	}
}

static void run_queue_next()
{
	krnl_task = hf_queue_remhead(krnl_run_queue);
	if (!krnl_task)
		panic(PANIC_NO_TASKS_RUN);
	if (hf_queue_addtail(krnl_run_queue, krnl_task))
		panic(PANIC_CANT_PLACE_RUN);
}

static void rt_queue_next()
{
	krnl_task = hf_queue_remhead(krnl_rt_queue);
	if (!krnl_task)
		panic(PANIC_NO_TASKS_RT);
	if (hf_queue_addtail(krnl_rt_queue, krnl_task))
		panic(PANIC_CANT_PLACE_RT);
}


/**
 * @brief Task dispatcher.
 * 
 * The job of the dispatcher is simple: save the current task context on the TCB,
 * update its state to ready and check its stack for overflow. If there are
 * tasks to be scheduled, process the delay queue and invoke the real-time scheduler.
 * If no RT tasks are ready to be scheduled, invoke the best effort scheduler.
 * Update the scheduled task state to running and restore the context of the task.
 * 
 * Delayed tasks are in the delay queue, and are processed in the following way:
 *	- The number of elements (tasks) in queue is counted;
 *	- The a task from the head of the queue is removed and its delay is decremented;
 * 		- If the decremented delay of a task reaches 0, it is put on RT or BE run queue;
 * 		- It is put it back on the tail of the delay queue otherwise;
 *	- Repeat until the whole queue is processed;
 */
 
void dispatch_isr(void *arg)
{
	int32_t rc;

#if KERNEL_LOG >= 1
	dprintf("dispatch %d ", (uint32_t)_read_us());
#endif
	_timer_reset();
	if (krnl_schedule == 0) return;
	krnl_task = &krnl_tcb[krnl_current_task];
	rc = setjmp(krnl_task->task_context);
	if (rc){
		return;
	}
	if (krnl_task->state == TASK_RUNNING)
		krnl_task->state = TASK_READY;
	if (krnl_task->pstack[0] != STACK_MAGIC)
		panic(PANIC_STACK_OVERFLOW);
	if (krnl_tasks > 0){
		process_delay_queue();
		krnl_current_task = krnl_pcb.sched_rt();
		if (krnl_current_task == 0)
			krnl_current_task = krnl_pcb.sched_be();
		krnl_task->state = TASK_RUNNING;
		krnl_pcb.preempt_cswitch++;
#if KERNEL_LOG >= 1
		dprintf("\n%d %d %d %d %d ", krnl_current_task, krnl_task->period, krnl_task->capacity, krnl_task->deadline, (uint32_t)_read_us());
#endif
		_restoreexec(krnl_task->task_context, 1, krnl_current_task);
		panic(PANIC_UNKNOWN);
	}else{
		panic(PANIC_NO_TASKS_LEFT);
	}
}

/**
 * @brief Best effort (BE) scheduler.
 * 
 * @return Best effort task id.
 * 
 * The algorithm is Round Robin.
 * 	- Take a task from the run queue, copy its entry and put it back at the tail of the run queue.
 * 	- If the task is in the blocked state (it may be simply blocked or waiting in a semaphore), it is
 * 	  put back at the tail of the run queue and the next task is picked up.
 * 	- So, if all tasks are blocked, at least the idle task can execute (it is never
 *	  blocked, at least it is what we hope!).
 * 	- Tasks in the blocked state are never removed from the run queue (they are
 *	  ignored), although they may be in another queue waiting for a resource.
 */
int32_t sched_rr(void)
{
	if (hf_queue_count(krnl_run_queue) == 0)
		panic(PANIC_NO_TASKS_RUN);
	do {
		run_queue_next();
	} while (krnl_task->state == TASK_BLOCKED);
	krnl_task->bgjobs++;

	return krnl_task->id;
}

/**
 * @brief Best effort (BE) scheduler.
 * 
 * @return Best effort task id.
 * 
 * The algorithm is Lottery Scheduling.
 * 	- Take a task from the run queue, copy its entry and put it back at the tail of the run queue.
 * 	- If the task is in the blocked state (it may be simply blocked or waiting in a semaphore) or
 * its not the ticket, it is put back at the tail of the run queue and the next task is picked up.
 */
int32_t sched_lottery(void)
{
	int32_t r, i = 0;
	
	r = random() % krnl_tasks;
	if (hf_queue_count(krnl_run_queue) == 0)
		panic(PANIC_NO_TASKS_RUN);
	do {
		run_queue_next();
	} while ((krnl_task->state == TASK_BLOCKED) || ((i++ % krnl_tasks) != r));
	krnl_task->bgjobs++;

	return krnl_task->id;
}

/**
 * @brief Best effort (BE) scheduler.
 * 
 * @return Best effort task id.
 * 
 * The algorithm is priority based Round Robin.
 * 	- Take the first task and put it at the end of the run queue (to advance the queue and avoid deadlocks)
 * 	- Perform a run in the queue, searching for the task with the highest priority (non blocked, lowest remaining priority)
 * 		- While we are at it, check if there is a critical task. If so, schedule it, and get out.
 * 	- Perform another run in the queue, updating the remaining priorities of all tasks by subtracting the priority
 * 	  of the task with the lowest remaining priority (task with the highest priority) from the remaining priority of
 * 	  all other tasks.
 */
int32_t sched_priorityrr(void)
{
	int32_t i, k;
	uint8_t highestp = 255;
	struct tcb_entry *krnl_task2 = NULL;
	
	k = hf_queue_count(krnl_run_queue);
	if (k == 0)
		panic(PANIC_NO_TASKS_RUN);

	/* advance the queue to prevent deadlocks */
	run_queue_next();

	/* search for the highest priority task */
	for (i = 0; i < k; i++){
		run_queue_next();
		/* critical event, bypass the queue */
		if (krnl_task->critical && krnl_task->state != TASK_BLOCKED){
			krnl_task->critical = 0;
			goto done;
		}
		if (highestp > krnl_task->priority_rem && krnl_task->state != TASK_BLOCKED){
			highestp = krnl_task->priority_rem;
			krnl_task2 = krnl_task;
		}
	}
	
	/* update priorities of all tasks */
	for (i = 0; i < k; i++){
		run_queue_next();
		if (krnl_task != krnl_task2)
			krnl_task->priority_rem -= krnl_task2->priority_rem;
	}

	krnl_task = krnl_task2;
	krnl_task->priority_rem = krnl_task->priority;
done:
	krnl_task->bgjobs++;
	
	return krnl_task->id;
}

/**
 * @brief Real time (RT) scheduler.
 * 
 * @return Real time task id.
 * 
 * The scheduling algorithm is Rate Monotonic.
 * 	- Sort the queue of RT tasks by period;
 * 	- Update real time information (remaining deadline and capacity) of the
 * whole task set.
 * 	- If the task at the head of the queue fits the requirements to be scheduled
 * (not blocked, has jobs to execute and no task with higher priority according to RM
 * was selected) then register the task to be scheduled.
 */

int32_t sched_rma(void)
{
	int32_t i, j, k;
	uint16_t id = 0;
	struct tcb_entry *e1, *e2;
	
	k = hf_queue_count(krnl_rt_queue);
	if (k == 0)
		return 0;
		
	for (i = 0; i < k-1; i++){
		for (j = i + 1; j < k; j++){
			e1 = hf_queue_get(krnl_rt_queue, i);
			e2 = hf_queue_get(krnl_rt_queue, j);
			if (e1->period > e2->period)
				if (hf_queue_swap(krnl_rt_queue, i, j))
					panic(PANIC_CANT_SWAP);
		}
	}

	for (i = 0; i < k; i++){
		rt_queue_next();
		if (krnl_task->state != TASK_BLOCKED && krnl_task->capacity_rem > 0 && !id){
			id = krnl_task->id;
			--krnl_task->capacity_rem;
		}
		if (--krnl_task->deadline_rem == 0){
			krnl_task->deadline_rem = krnl_task->period;
			if (krnl_task->capacity_rem > 0) krnl_task->deadline_misses++;
			krnl_task->capacity_rem = krnl_task->capacity;
		}
	}

	if (id){
		krnl_task = &krnl_tcb[id];
		krnl_task->rtjobs++;
		return id;
	}else{
		/* no RT task to run */
		krnl_task = &krnl_tcb[0];
		return 0;
	}
}
