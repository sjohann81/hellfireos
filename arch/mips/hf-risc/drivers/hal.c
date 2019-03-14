#include <hellfire.h>
#ifdef USTACK
#include <ustack.h>
#endif

/* hardware dependent C library stuff */
#ifndef DEBUG_PORT
void putchar(int32_t value)
{
	while (UARTCAUSE & MASK_UART0_WRITEBUSY);
	UART0 = value;
}

int32_t kbhit(void)
{
	return UARTCAUSE & MASK_UART0_DATAAVAIL;
}

int32_t getchar(void)
{
	while (!kbhit());
	return UART0;
}
#else
void putchar(int32_t value)
{
	DEBUG_ADDR = value;
}

int32_t kbhit(void)
{
	return 0;
}

int32_t getchar(void)
{
	return DEBUG_ADDR;
}
#endif

void dputchar(int32_t value)
{
	DEBUG_ADDR = value;
}

/* hardware platform dependent stuff */
void delay_ms(uint32_t msec)
{
	volatile uint32_t cur, last, delta, msecs;
	uint32_t cycles_per_msec;

	last = TIMER0;
	delta = msecs = 0;
	cycles_per_msec = CPU_SPEED / 1000;
	while (msec > msecs) {
		cur = TIMER0;
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

	last = TIMER0;
	delta = usecs = 0;
	cycles_per_usec = CPU_SPEED / 1000000;
	while (usec > usecs) {
		cur = TIMER0;
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
	uint16_t d;

	d = (uint16_t)(CPU_SPEED / baud);
	UART0DIV = d;
	UART0 = 0;

	PAALTCFG0 |= MASK_UART0;
}

static void ioports_init(void)
{
}

void led_set(uint16_t led, uint8_t val)
{
}

uint8_t button_get(uint16_t btn)
{
	return 0;
}

uint8_t switch_get(uint16_t sw)
{
	return 0;
}

/* hardware dependent basic kernel stuff */
void _cpu_idle(void)
{
}

static void _idletask(void)
{
	for (;;){
		_cpu_idle();
	}
}

void _hardware_init(void)
{
	ioports_init();
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

void timer0b_handler(void)
{
	dispatch_isr(0);
}

void timer1ctc_handler(void)
{
	dispatch_isr(0);
}

void _timer_init(void)
{
	kprintf("\nHAL: _timer_init()");
#if TIME_SLICE == 0
	TIMERMASK |= MASK_TIMER0B;
#else
	TIMER1PRE = TIMERPRE_DIV64;
	/* unlock TIMER1 for reset */
	TIMER1 = TIMERSET;
	TIMER1 = 0;
	/* timer1 divisor is 64, so divide everything by 64 */
	TIMER1CTC = ((CPU_SPEED / 1000000) * TIME_SLICE) >> 6;

	TIMERMASK |= MASK_TIMER1CTC;
#endif
}

void _irq_init(void)
{
	kprintf("\nHAL: _irq_init()");
	/* enable mask for Segment 0 (tied to IRQ0 line) */
	IRQ_MASK = MASK_IRQ0;
	/* global interrupts enable */
	IRQ_STATUS = 1;
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
}

void _set_task_sp(uint16_t task, size_t stack)
{
	krnl_tcb[task].task_context[10] = stack;
}

size_t _get_task_sp(uint16_t task)
{
	return krnl_tcb[task].task_context[10];
}

void _set_task_tp(uint16_t task, void (*entry)())
{
	krnl_tcb[task].task_context[11] = (size_t)entry;
}

void *_get_task_tp(uint16_t task)
{
	return (void *)krnl_tcb[task].task_context[11];
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
	return TIMER0;
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

void _panic(void)
{
	volatile uint32_t *trap_addr = (uint32_t *)0xe0000000;
	*trap_addr = 0;
}
