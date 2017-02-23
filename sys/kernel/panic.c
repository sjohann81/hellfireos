/**
 * @file panic.c
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
 * Kernel panic.
 */

#include <hal.h>
#include <libc.h>
#include <kprintf.h>
#include <kernel.h>
#include <panic.h>
#include <scheduler.h>


/**
 * @internal
 * @brief Causes the kernel to panic.
 * 
 * Interrupts are disabled, the panic cause is presented to the user and the
 * system is locked forever.
 */
void panic(int32_t cause)
{
	_di();
#if KERNEL_LOG == 1
	dprintf("%d %d panic()", (uint32_t)_read_us(), krnl_current_task);
#endif	
	kprintf("\nKERNEL: panic [task %d] - ", krnl_current_task);
	switch(cause){
	case PANIC_ABORTED:		kprintf("execution aborted"); break;
	case PANIC_GPF:			kprintf("general protection fault"); break;
	case PANIC_STACK_OVERFLOW:	kprintf("stack overflow"); break;
	case PANIC_STACK_CORRUPT:	kprintf("stack is corrupted"); break;
	case PANIC_NO_TASKS_LEFT:	kprintf("no more tasks left to dispatch"); break;
	case PANIC_OOM:			kprintf("out of memory"); break;
	case PANIC_NO_TASKS_RUN:	kprintf("no tasks on run queue"); break;
	case PANIC_NO_TASKS_DELAY:	kprintf("no tasks on delay queue"); break;
	case PANIC_NO_TASKS_RT:		kprintf("no tasks on realtime queue"); break;
	case PANIC_UNKNOWN_TASK_STATE:	kprintf("task in unknown state"); break;
	case PANIC_CANT_PLACE_RUN:	kprintf("can't place task on run queue"); break;
	case PANIC_CANT_PLACE_DELAY:	kprintf("can't place task on delay queue"); break;
	case PANIC_CANT_PLACE_RT:	kprintf("can't place task on real time queue"); break;
	case PANIC_CANT_SWAP:		kprintf("can't swap tasks on queue"); break;
	case PANIC_NUTS_SEM:		kprintf("insane semaphore"); break;
	default:			kprintf("unknown error"); break;
	}
	printf(" -> system halted.\n");
	_panic();
	for(;;);
}
