/**
 * @file main.c
 * @author Sergio Johann Filho
 * @date January 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * The HellfireOS realtime operating system kernel.
 * 
 */
 
#include <hellfire.h>

static void print_config(void)
{
	kprintf("\n===========================================================");
	kprintf("\nHellfireOS %s [%s, %s]", KERN_VER, __DATE__, __TIME__);
	kprintf("\nEmbedded Systems Group - GSE, PUCRS - [2007 - 2016]");
	kprintf("\n===========================================================\n");
	kprintf("\ncpu id:        %d", CPU_ID);
	kprintf("\narch:          %s", CPU_ARCH);
	kprintf("\nsys clk:       %d kHz", CPU_SPEED/1000);
	if (TIME_SLICE != 0)
		kprintf("\ntime slice:    %d us", TIME_SLICE);
	kprintf("\nheap size:     %d bytes", sizeof(krnl_heap));
	kprintf("\nmax tasks:     %d\n", MAX_TASKS);
}

static void clear_tcb(void)
{
	uint16_t i, j;
	
	for(i = 0; i < MAX_TASKS; i++){
		krnl_task = &krnl_tcb[i];
		krnl_task->id = -1;
		for (j = 0; j < sizeof(krnl_task->name); j++)
			krnl_task->name[j] = 0;
		krnl_task->state = TASK_IDLE;
		krnl_task->jobs = 0;
		krnl_task->deadline_misses = 0;
		krnl_task->period = 0;
		krnl_task->capacity = 0;
		krnl_task->deadline = 0;
		krnl_task->capacity_rem = 0;
		krnl_task->deadline_rem = 0;
		krnl_task->ptask = NULL;
		krnl_task->pstack = NULL;
		krnl_task->stack_size = 0;
		krnl_task->other_data = 0;
	}

	krnl_tasks = 0;
	krnl_current_task = 0;
	krnl_schedule = 0;
}

static void init_queues(void)
{
	krnl_run_queue = hf_queue_create(MAX_TASKS);
	if (krnl_run_queue == NULL) panic(PANIC_OOM);
	krnl_delay_queue = hf_queue_create(MAX_TASKS);
	if (krnl_delay_queue == NULL) panic(PANIC_OOM);
	krnl_rt_queue = hf_queue_create(MAX_TASKS);
	if (krnl_rt_queue == NULL) panic(PANIC_OOM);
}

static void idletask(void)
{
	hf_schedlock(0);
	for (;;){
		_cpu_idle();
	}
}

/**
 * @internal
 * @brief HellfireOS kernel entry point and system initialization.
 * 
 * @return should not return.
 * 
 * We assume that the following machine state has been already set
 * before this routine.
 *	- Kernel BSS section is filled with 0.
 *	- Kernel stack is configured.
 *	- All interrupts are disabled.
 *	- Minimum page table is set. (MMU systems only)
 */
int main(void)
{
	static uint32_t oops=0xbaadd00d;
	
	_hardware_init();
	hf_schedlock(1);
	_di();
	if (oops == 0xbaadd00d){
		oops = 0;
		print_config();
		_vm_init();
		clear_tcb();
		init_queues();
		_sched_init();
		_device_init();
		_irq_init();
		_timer_init();
		hf_spawn(idletask, 0, 0, 0, "idle task", 1024);
		_di();
		_task_init();
		app_main();
		sched_be();
		kprintf("\nfree heap: %d bytes", krnl_free);
		kprintf("\nKERNEL: HellfireOS is running\n");
		_restoreexec(krnl_tcb[0].task_context, 1, 0);
		panic(PANIC_ABORTED);
	}else{
		panic(PANIC_GPF);
	}
	
	return 0;
}
