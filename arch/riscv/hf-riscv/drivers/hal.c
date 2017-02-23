#include <hellfire.h>
#ifdef USTACK
#include <ustack.h>
#endif

/* hardware dependent C library stuff */
#ifndef DEBUG_PORT
void putchar(int32_t value)
{
	while ((IRQ_CAUSE & IRQ_UART_WRITE_AVAILABLE) == 0);
	UART = value;
}

int32_t kbhit(void)
{
	return IRQ_CAUSE & IRQ_UART_READ_AVAILABLE;
}

int32_t getchar(void)
{
	while (!kbhit());
	return UART;
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

	last = COUNTER;
	delta = msecs = 0;
	cycles_per_msec = CPU_SPEED / 1000;
	while(msec > msecs){
		cur = COUNTER;
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

	last = COUNTER;
	delta = usecs = 0;
	cycles_per_usec = CPU_SPEED / 1000000;
	while(usec > usecs){
		cur = COUNTER;
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

static void uart_init(uint32_t baud)
{
	uint16_t d;

	d = (uint16_t)(CPU_SPEED / baud);
	UART_DIVISOR = d;
	d = UART;
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
void _hardware_init(void)
{
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
	kprintf("\nHAL: _timer_init()");
#if TIME_SLICE == 0
	_irq_register(IRQ_COUNTER, dispatch_isr);
	_irq_register(IRQ_COUNTER_NOT, dispatch_isr);
	IRQ_MASK = IRQ_COUNTER;
#else
	_irq_register(IRQ_COMPARE2, dispatch_isr);
	COMPARE2 = COUNTER + (CPU_SPEED/1000000) * TIME_SLICE;
	IRQ_MASK = IRQ_COMPARE2;
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
#ifdef USTACK
	ustack_init();
#endif
}

void _set_task_sp(uint16_t task, size_t stack)
{
	krnl_tcb[task].task_context[12] = stack;
}

size_t _get_task_sp(uint16_t task)
{
	return krnl_tcb[task].task_context[12];
}

void _set_task_tp(uint16_t task, void (*entry)())
{
	krnl_tcb[task].task_context[13] = (size_t)entry;
}

void *_get_task_tp(uint16_t task)
{
	return (void *)krnl_tcb[task].task_context[13];
}

void _timer_reset(void)
{
	static uint32_t timecount, lastcount = 0;
	
#if TIME_SLICE == 0
	uint32_t m;

	m = IRQ_MASK;							// read interrupt mask
	m ^= (IRQ_COUNTER | IRQ_COUNTER_NOT);				// toggle timer interrupt mask
	IRQ_MASK = m;
#else
	uint32_t val;

	val = COUNTER;
	val += (CPU_SPEED/1000000) * TIME_SLICE;
	COMPARE2 = val;
#endif
	timecount = _read_us();
	krnl_pcb.tick_time = timecount - lastcount;
	lastcount = timecount;
}

void _cpu_idle(void)
{
}

uint32_t _readcounter(void)
{
	return COUNTER;
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
