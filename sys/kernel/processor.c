/**
 * @file processor.c
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
 * Processor and scheduler management primitives and auxiliary functions.
 * 
 */

#include <hal.h>
#include <libc.h>
#include <kprintf.h>
#include <kernel.h>
#include <ecodes.h>

/**
 * @brief Enables or disables the task scheduler.
 * 
 * @param lock defines the scheduler activation (a value of 1 disables task scheduling).
 */
void hf_schedlock(int32_t lock)
{
#if KERNEL_LOG == 2
	dprintf("hf_schedlock() %d ", (uint32_t)_read_us());
#endif
	if (lock)
		krnl_schedule = 0;
	else
		krnl_schedule = 1;
}

/**
 * @brief Returns the percentage of free processor time. Only realtime tasks
 * are accounted as processor load.
 * 
 * @return a number representing the percentage of free processor.
 */
int32_t hf_freecpu(void)
{
	int32_t i, s = 0;
	struct tcb_entry *krnl_task2;

#if KERNEL_LOG == 2
	dprintf("hf_freecpu() %d ", (uint32_t)_read_us());
#endif
	for (i = 0; i < MAX_TASKS; i++){
		krnl_task2 = &krnl_tcb[i];
		if (krnl_task2->ptask){
			if (krnl_task2->state != TASK_BLOCKED && krnl_task2->period)
				s += (krnl_task2->capacity * 100) / krnl_task2->period;
		}
	}
	
	return 100 - s;
}

/**
 * @brief Returns the percentage of processor time used by a given task. Both realtime
 * and best effort tasks are accounted. Best effort tasks lose processor time when realtime
 * tasks are part of the task set, as they only run in the background (idle time).
 * 
 * @param id is the task id number
 * 
 * @return a number representing the percentage of processor usage or ERR_INVALID_ID if
 * the referenced task does not exist.
 */
int32_t hf_cpuload(uint16_t id)
{
	int32_t i, s = 0, n;
	struct tcb_entry *krnl_task2;

#if KERNEL_LOG == 2
	dprintf("hf_cpuload() %d ", (uint32_t)_read_us());
#endif
	for (i = 0; i < MAX_TASKS; i++){
		krnl_task2 = &krnl_tcb[i];
		if (krnl_task2->ptask){
			if (krnl_task2->state != TASK_BLOCKED){
				if (krnl_task2->period)
					s += krnl_task2->rtjobs;
				else
					s += krnl_task2->bgjobs;
			}
		}
	}
	
	if (id < MAX_TASKS){
		if (krnl_tcb[id].ptask){
			if (krnl_tcb[id].period)
				n = (krnl_tcb[id].rtjobs * 100) / s;
			else
				n = (krnl_tcb[id].bgjobs * 100) / s;
			
			return n;
		}
	}

	return ERR_INVALID_ID;
}

/**
 * @brief Returns the amount of free memory.
 * 
 * @return free heap memory, in bytes.
 */
uint32_t hf_freemem(void)
{
#if KERNEL_LOG == 2
	dprintf("hf_freemem() %d ", (uint32_t)_read_us());
#endif
	return krnl_free;
}

uint32_t hf_ticktime(void)
{
#if KERNEL_LOG == 2
	dprintf("hf_ticktime() %d ", (uint32_t)_read_us());
#endif
	return krnl_pcb.tick_time;
}
