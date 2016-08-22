#include <hellfire.h>

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

void uart_init(uint32_t baud)
{
	uint16_t d;

	d = (uint16_t)(CPU_SPEED / baud);
	UART_DIVISOR = d;
	d = UART;
}

/* hardware dependent basic kernel stuff */
void _hardware_init(void)
{
#ifndef DEBUG_PORT
//	uart_init(57600);
#endif
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
	_irq_register(IRQ_COUNTER2, dispatch_isr);
	_irq_register(IRQ_COUNTER2_NOT, dispatch_isr);
	IRQ_MASK = IRQ_COUNTER2;
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
#endif
}

void _task_init(void)
{
	kprintf("\nHAL: _task_init()");
}

void _set_task_sp(uint16_t task, uint32_t stack)
{
	krnl_tcb[task].task_context[10] = stack;
}

uint32_t _get_task_sp(uint16_t task)
{
	return krnl_tcb[task].task_context[10];
}

void _set_task_tp(uint16_t task, void (*entry)())
{
	krnl_tcb[task].task_context[11] = (uint32_t)entry;
}

void *_get_task_tp(uint16_t task)
{
	return (void *)krnl_tcb[task].task_context[11];
}

void _timer_reset(void)
{
#if TIME_SLICE == 0
	uint32_t m;

	m = IRQ_MASK;							// read interrupt mask
	m ^= (IRQ_COUNTER2 | IRQ_COUNTER2_NOT);				// toggle timer interrupt mask
	IRQ_MASK = m;
#else
	uint32_t val;

	val = COUNTER;
	val += (CPU_SPEED/1000000) * TIME_SLICE;
	COMPARE2 = val;
#endif
	_read_us();
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
