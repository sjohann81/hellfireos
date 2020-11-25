#include <hellfire.h>
#ifdef USTACK
#include <ustack.h>
#endif

/* hardware dependent C library stuff */
#ifndef DEBUG_PORT
void putchar(int32_t value)
{
	while((MemoryRead(IRQ_STATUS) & IRQ_UART_WRITE_AVAILABLE) == 0);
	MemoryWrite(UART_WRITE, value);
}

int32_t kbhit(void)
{
	return MemoryRead(IRQ_STATUS) & IRQ_UART_READ_AVAILABLE;
}

int32_t getchar(void)
{
	while (!kbhit());
	return MemoryRead(UART_READ);
}
#else
void putchar(int32_t value)
{
	MemoryWrite(OUT_FACILITY, value);
}

int32_t kbhit(void)
{
	return 0;
}

int32_t getchar(void)
{
	return 0;
}
#endif

void dputchar(int32_t value){
}

/* hardware platform dependent stuff */
void delay_ms(uint32_t msec)
{
	volatile uint32_t cur, last, delta, msecs;
	uint32_t cycles_per_msec;

	last = MemoryRead(COUNTER_REG);
	delta = msecs = 0;
	cycles_per_msec = CPU_SPEED / 1000;
	while(msec > msecs){
		cur = MemoryRead(COUNTER_REG);
		if (cur < last)
			delta += (cur + (CPU_SPEED - last));
		else
			delta += (cur - last);
		last = cur;
		if (delta >= cycles_per_msec){
			msecs += delta / cycles_per_msec;
			delta %= cycles_per_msec;
		}
	}
}

void delay_us(uint32_t usec)
{
	volatile uint32_t cur, last, delta, usecs;
	uint32_t cycles_per_usec;

	last = MemoryRead(COUNTER_REG);
	delta = usecs = 0;
	cycles_per_usec = CPU_SPEED / 1000000;
	while(usec > usecs){
		cur = MemoryRead(COUNTER_REG);
		if (cur < last)
			delta += (cur + (CPU_SPEED - last));
		else
			delta += (cur - last);
		last = cur;
		if (delta >= cycles_per_usec){
			usecs += delta / cycles_per_usec;
			delta %= cycles_per_usec;
		}
	}
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
	if(!(MemoryRead(LOG_FACILITY) != 0xa5a5a5a5)){						// detect if running on simulator
		MemoryWrite(FREQUENCY_REG, CPU_SPEED);						// set MPSoC frequency register
		MemoryWrite(TICK_TIME_REG, 1<<TICK_TIME);					// set tick time register
	}
	MemoryWrite(IRQ_MASK, 0);
	ioports_init();
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

void _timer_init(void)
{
	uint32_t m;

	kprintf("\nHAL: _timer_init()");
	_irq_register(IRQ_COUNTER18, dispatch_isr);
	_irq_register(IRQ_COUNTER18_NOT, dispatch_isr);
	m = MemoryRead(IRQ_MASK);
	m |= IRQ_COUNTER18;
	MemoryWrite(IRQ_MASK, m);
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
	uint32_t m;

	m = MemoryRead(IRQ_MASK);						// read interrupt mask
	m ^= (IRQ_COUNTER18 | IRQ_COUNTER18_NOT);				// toggle timer interrupt mask
	MemoryWrite(IRQ_MASK, m);
	timecount = _read_us();
	krnl_pcb.tick_time = timecount - lastcount;
	lastcount = timecount;
}

uint32_t _readcounter(void)
{
	return MemoryRead(COUNTER_REG);
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
	volatile uint32_t *trap_addr = (uint32_t *)EXIT_TRAP;
	*trap_addr = 0;
}
