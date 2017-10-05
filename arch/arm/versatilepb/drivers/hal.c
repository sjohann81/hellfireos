#include <hellfire.h>
#ifdef USTACK
#include <ustack.h>
#endif

/* hardware dependent C library stuff */
void putchar(int32_t value)
{
	while (UART0FR & UARTFR_TXFF);
	UART0DR = value;
}

int32_t kbhit(void)
{
	while (~(UART0FR & UARTFR_TXFF));
}

int32_t getchar(void)
{
	return UART0DR;
}

void dputchar(int32_t value)
{
}

/* hardware platform dependent stuff */
/*
void _ei(uint32_t status)
{
	VIC_IRQENABLE = status;
}
 
uint32_t _di(void)
{
	uint32_t status;

	status = VIC_IRQENABLE;
	VIC_IRQENCLEAR = status;

	return status;
}
*/
void delay_ms(uint32_t msec)
{
	volatile uint32_t cur, last, delta, msecs;
	uint32_t cycles_per_msec;

	last = _readcounter();
	delta = msecs = 0;
	cycles_per_msec = 1000;
	while(msec > msecs){
		cur = _readcounter();
		if (cur < last)
			delta += (cur + (1000000 - last));
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

	last = _readcounter();
	delta = usecs = 0;
	cycles_per_usec = 1;
	while(usec > usecs){
		cur = _readcounter();
		if (cur < last)
			delta += (cur + (1000000 - last));
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
	TIMER0_LOAD = 10000;
#else
	TIMER0_LOAD = TIME_SLICE;
#endif
	TIMER0_CONTROL = TIMER_EN | TIMER_PERIODIC | TIMER_32BIT | TIMER_INTEN;
	TIMER3_CONTROL = TIMER_EN | TIMER_32BIT;
}

void _irq_init(void)
{
	kprintf("\nHAL: _irq_init()");
	
	_irq_register(INTMASK_TIMERINT0_1, dispatch_isr);
	VIC_IRQENABLE = INTMASK_TIMERINT0_1;
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
	krnl_tcb[task].task_context[8] = stack;
}

size_t _get_task_sp(uint16_t task)
{
	return krnl_tcb[task].task_context[8];
}

void _set_task_tp(uint16_t task, void (*entry)())
{
	krnl_tcb[task].task_context[9] = (size_t)entry;
}

void *_get_task_tp(uint16_t task)
{
	return (void *)krnl_tcb[task].task_context[9];
}

void _timer_reset(void)
{
	static uint32_t timecount, lastcount = 0;
	
	if (TIMER0_MIS){
		TIMER0_INTCLR = 1;
		
		timecount = _read_us();
		krnl_pcb.tick_time = timecount - lastcount;
		lastcount = timecount;
	}
}

void _cpu_idle(void)
{
}

uint32_t _readcounter(void)
{
	return ~TIMER3_VALUE;
}

uint64_t _read_us(void)
{
	static uint64_t timeref = 0;
	static uint32_t tval2 = 0, tref = 0;
	
	if (tref == 0) _readcounter();
	if (_readcounter() < tref) tval2++;
	tref = _readcounter();
	timeref = ((uint64_t)tval2 << 32) + (uint64_t)_readcounter();
	
	return timeref;
}

void _panic(void)
{
}

