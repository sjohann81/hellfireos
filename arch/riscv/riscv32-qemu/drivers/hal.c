#include <hellfire.h>
#ifdef USTACK
#include <ustack.h>
#endif

/* hardware dependent C library stuff */
void putchar(int32_t value)
{
	while (!((NS16550A_UART0_CTRL_ADDR(NS16550A_LSR) & NS16550A_LSR_RI)));
	NS16550A_UART0_CTRL_ADDR(NS16550A_THR) = value;
}

int32_t kbhit(void)
{
	return 0;
}

int32_t getchar(void)
{
	return 0;
}

void dputchar(int32_t value)
{
}

/* hardware platform dependent stuff */
int32_t _interrupt_set(int32_t s)
{
	volatile int32_t val;
	
	val = read_csr(mstatus) & 0x8;
	if (s) {
		asm volatile ("csrs mstatus, 8");
	} else {
		asm volatile ("csrc mstatus, 8");
	}

	return val;
}

void delay_ms(uint32_t msec)
{
	volatile uint32_t cur, last, delta, msecs;
	uint32_t cycles_per_msec;

	last = MTIME_L;
	delta = msecs = 0;
	cycles_per_msec = CPU_SPEED / 1000;
	while (msec > msecs) {
		cur = MTIME_L;
		if (cur < last)
			delta += (cur + (CPU_SPEED - last));
		else
			delta += (cur - last);
		last = cur;
		if (delta >= cycles_per_msec) {
			msecs += delta / cycles_per_msec;
			delta %= cycles_per_msec;
		}
	}
}

void delay_us(uint32_t usec)
{
	volatile uint32_t cur, last, delta, usecs;
	uint32_t cycles_per_usec;

	last = MTIME_L;
	delta = usecs = 0;
	cycles_per_usec = CPU_SPEED / 1000000;
	while (usec > usecs) {
		cur = MTIME_L;
		if (cur < last)
			delta += (cur + (CPU_SPEED - last));
		else
			delta += (cur - last);
		last = cur;
		if (delta >= cycles_per_usec) {
			usecs += delta / cycles_per_usec;
			delta %= cycles_per_usec;
		}
	}
}

static void uart_init(uint32_t baud)
{
	uint32_t divisor = CPU_SPEED / (16 * baud);

	NS16550A_UART0_CTRL_ADDR(NS16550A_LCR) = NS16550A_LCR_DLAB;
	NS16550A_UART0_CTRL_ADDR(NS16550A_DLM) = (divisor >> 8) & 0xff;
	NS16550A_UART0_CTRL_ADDR(NS16550A_DLL) = divisor & 0xff;
	NS16550A_UART0_CTRL_ADDR(NS16550A_LCR) = NS16550A_LCR_8BIT;
}

/* hardware dependent basic kernel stuff */
void _cpu_idle(void)
{
	asm volatile ("wfi");
}

static void _idletask(void)
{
	for (;;){
		_cpu_idle();
	}
}

void _hardware_init(void)
{
	uart_init(TERM_BAUD);
}

void _vm_init(void)
{
	kprintf("\nHAL: _vm_init()");
	heapinit(krnl_heap, sizeof(krnl_heap));
}

void _sched_init(void)
{
	kprintf("\nHAL: _sched_init()");
}

//void timer_handler(void)
void _irq_handler(uint32_t cause, uint32_t *stack)
{
	uint32_t val;
	
	val = read_csr(mcause);
	if (mtime_r() > mtimecmp_r()) {
#if TIME_SLICE == 0
		mtimecmp_w(mtime_r() + 0x1ffff);
#endif
		dispatch_isr(0);
	} else {
		printf("[%x]\n", val);
		for (;;);
	}

}

void _timer_init(void)
{
	kprintf("\nHAL: _timer_init()");
	
#if TIME_SLICE == 0
	mtimecmp_w(mtime_r() + 0x1ffff);
	write_csr(mie, 128);
#endif
}

void _irq_init(void)
{
	kprintf("\nHAL: _irq_init()");
}

void _device_init(void)
{
	kprintf("\nHAL: _device_init()");
#ifdef NOC_INTERCONNECT
	ni_init();
#endif
#ifdef USTACK
	en_init();
#endif
}

void _task_init(void)
{
	kprintf("\nHAL: _task_init()");

	hf_spawn(_idletask, 0, 0, 0, "idle task", 1024);
#ifdef USTACK
	ustack_init();
#endif
	app_main();

	kprintf("\nKERNEL: free heap: %d bytes", krnl_free);
	kprintf("\nKERNEL: HellfireOS is up\n");

	krnl_task = &krnl_tcb[0];
	hf_schedlock(0);
	_context_restore(krnl_task->task_context, 1);
	for (;;);
}

void _set_task_sp(uint16_t task, size_t stack)
{
	krnl_tcb[task].task_context[14] = stack;
}

size_t _get_task_sp(uint16_t task)
{
	return krnl_tcb[task].task_context[14];
}

void _set_task_tp(uint16_t task, void (*entry)())
{
	krnl_tcb[task].task_context[15] = (size_t)entry;
}

void *_get_task_tp(uint16_t task)
{
	return (void *)krnl_tcb[task].task_context[15];
}

void _timer_reset(void)
{
	static uint32_t timecount, lastcount = 0;

	timecount = _read_us();
	krnl_pcb.tick_time = timecount - lastcount;
	lastcount = timecount;
}

uint32_t _readcounter(void)
{
	return MTIME_L;
}

uint64_t _read_us(void)
{
	static uint64_t timeref = 0;
	static uint32_t tval2 = 0, tref = 0;

	if (tref == 0) _readcounter();
	if (_readcounter() < tref) tval2++;
	tref = _readcounter();
	timeref = ((uint64_t)tval2 << 32) + (uint64_t)_readcounter();

	return (timeref / (CPU_SPEED / 1000000));
}

// https://forums.sifive.com/t/timer-and-interrupt/3456/5
uint64_t mtime_r(void)
{
	uint32_t hi, lo;
	
	while (1) {
		hi = MTIME_H;
		lo = MTIME_L;
		
		if (hi == MTIME_H)
			return ((uint64_t) hi << 32) | lo;
	}
}

void mtime_w(uint64_t val)
{
	MTIME_H = 0;
	MTIME_L = (uint32_t)(val & 0xffffffff);
	MTIME_H = MTIME_H + (uint32_t)(val >> 32);
}

uint64_t mtimecmp_r(void)
{
	uint32_t hi, lo;
	
	hi = MTIMECMP_H;
	lo = MTIMECMP_L;
	
	return ((uint64_t) hi << 32) | lo;
}

void mtimecmp_w(uint64_t val)
{
	MTIMECMP_L = -1;
	MTIMECMP_H = (uint32_t)((val >> 32) & 0xffffffff);
	MTIMECMP_L = (uint32_t)(val & 0xffffffff);
}

void _panic(void)
{
	volatile int * const exit_device = (int* const)0x100000;
	*exit_device = 0x5555;
	while (1);
}
