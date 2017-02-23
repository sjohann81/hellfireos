#include <hellfire.h>
#ifdef USTACK
#include <ustack.h>
#endif

PIC32MZ_DEVCFG (
    _DEVCFG0_JTAG_DISABLE |      /* Disable JTAG port */
    _DEVCFG0_TRC_DISABLE,        /* Disable trace port */
    _DEVCFG1_FNOSC_SPLL |        /* System clock supplied by SPLL */
    _DEVCFG1_POSCMOD_DISABLE |   /* Primary oscillator disabled */
    _DEVCFG1_CLKO_DISABLE,       /* CLKO output disable */

    _DEVCFG2_FPLLIDIV_1 |        /* PLL input divider = 1 */
    _DEVCFG2_FPLLRNG_5_10 |      /* PLL input range is 5-10 MHz */
    _DEVCFG2_FPLLICLK_FRC |      /* Select FRC as input to PLL */
    _DEVCFG2_FPLLMULT(50) |      /* PLL multiplier = 50x */
    _DEVCFG2_FPLLODIV_2,         /* PLL postscaler = 1/2 */
    _DEVCFG3_FETHIO |            /* Default Ethernet pins */
    _DEVCFG3_USERID(0xffff));    /* User-defined ID */

/* hardware dependent C library stuff */
void putchar(int32_t value)
{
	while (U2STA & USTA_UTXBF);
	U2TXREG = value;	
}

int32_t kbhit(void)
{
        return (U2STA & USTA_URXDA);
}

int32_t getchar(void)
{
	while (!kbhit());
	return (uint8_t)U2RXREG;
}

void dputchar(int32_t value){
}

/* hardware platform dependent stuff */
void delay_ms(uint32_t msec)
{
	uint32_t now = mfc0(CP0_COUNT, 0);
	uint32_t final = now + msec * (CPU_SPEED / 1000) / 2;

	for (;;){
		now = mfc0(CP0_COUNT, 0);
		if ((int32_t) (now - final) >= 0) break;
	}
}

void delay_us(uint32_t usec)
{
	uint32_t now = mfc0(CP0_COUNT, 0);
	uint32_t final = now + usec * (CPU_SPEED / 1000000) / 2;

	for (;;){
		now = mfc0(CP0_COUNT, 0);
		if ((int32_t) (now - final) >= 0) break;
	}
}

static void uart_init(uint32_t baud)
{
	/* Initialize UART. */
	U2BRG = BRG_BAUD (CPU_SPEED / 2, baud);
	U2STA = 0;
	U2MODE = UMODE_PDSEL_8NPAR |		/* 8-bit data, no parity */
		UMODE_ON;			/* UART Enable */
	U2STASET = USTA_URXEN |	USTA_UTXEN;	/* RX / TX Enable */
}

static void ioports_init(void)
{
	// TODO: init leds, buttons and switches
	
	TRISBSET =	(1 << 12);		/* SW1 - RB12 (active low) - this will be used as a RESET button */
  	CNPUB = 	(1 << 12);		/* enable pull-up */
	
	TRISKCLR = (1 << 1) | (1 << 2) | (1 << 3);	/* software SPI (outputs): RK1 (cs), RK2 (sck), RK3 (mosi) */
	TRISKSET = (1 << 4) | (1 << 5);			/* software SPI (inputs) : RK4 (miso) RK5 (irq) */
}

uint32_t _port_read(uint32_t port)
{
	uint32_t data = 0;
	
	switch(port){
		case RA:
			data = PORTA;
			break;
		case RB:
			data = PORTB;
			break;
		case RC:
			data = PORTC;
			break;
		case RD:
			data = PORTD;
			break;
		case RE:
			data = PORTE;
			break;
		case RF:
			data = PORTF;
			break;
		case RG:
			data = PORTG;
			break;
		case RH:
			data = PORTH;
			break;
		case RJ:
			data = PORTJ;
			break;
		case RK:
			data = PORTK;
			break;
		default:
			break;
	}
	return data;
}

void _port_write(uint32_t port, uint32_t val)
{
	switch(port){
		case RA:
			LATA = val;
			break;
		case RB:
			LATB = val;
			break;
		case RC:
			LATC = val;
			break;
		case RD:
			LATD = val;
			break;
		case RE:
			LATE = val;
			break;
		case RF:
			LATF = val;
			break;
		case RG:
			LATG = val;
			break;
		case RH:
			LATH = val;
			break;
		case RJ:
			LATJ = val;
			break;
		case RK:
			LATK = val;
			break;
		default:
			break;
	}
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
	uint32_t temp_CP0;
	
	/* configure board registers (clock source, multiplier...) */
//	SYSKEY = 0xAA996655;
//	SYSKEY = 0x556699AA;
//
//	SYSKEY = 0x33333333;
//	while (OSCCON & 0x1);
//	PB5DIV = (PB5DIV & 0xff80) | 0x2;	/* 50MHz for Ethernet */
	
	ioports_init();
	uart_init(TERM_BAUD);
	
	/* configure the interrupt controller to compatibility mode */
	asm volatile("di");			/* Disable all interrupts */
	mtc0 (CP0_EBASE, 1, 0x9d000000);	/* Set an EBase value of 0x9D000000 */
	temp_CP0 = mfc0(CP0_CAUSE, 0);		/* Get Cause */
	temp_CP0 |= CAUSE_IV;			/* Set Cause IV */
	mtc0(CP0_CAUSE, 0, temp_CP0);		/* Update Cause */
	INTCONCLR = INTCON_MVEC;		/* Clear the MVEC bit */
	temp_CP0 = mfc0(CP0_STATUS, 0);		/* Get Status */
	temp_CP0 &= ~STATUS_BEV;		/* Clear Status IV */
	mtc0(CP0_STATUS, 0, temp_CP0);		/* Update Status */
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
	uint32_t val;
	
	kprintf("\nHAL: _timer_init()");
#if TIME_SLICE == 0
	_irq_register(0, 0x00000200, dispatch_isr);
	/* setup timer2 - check PIC32MZ datasheet, page 118, section 7.2 */
	T2CON = 0x0;
	TMR2 = 0x0;
	PR2 = 0xffff;
	IPCSET(2) = 0x00001f00;
	IFSCLR(0) = 0x00000200;
	IECSET(0) = 0x00000200;
	T2CON |= 0x8000;
#else
	/* TIME_SLICE is in microsseconds */
	val = (CPU_SPEED / 2) / (1000000 / TIME_SLICE);
	_irq_register(0, 0x00004000, dispatch_isr);
	T2CON = 0x8;
	TMR2 = 0x0;
	TMR3 = 0x0;
	PR2 = val & 0xffff;
	PR3 = (val >> 16) & 0xffff;
	IPCSET(3) = 0x001f0000;
	IFSCLR(0) = 0x00004000;
	IECSET(0) = 0x00004000;
	T2CON |= 0x8000;
#endif
}

void _irq_init(void)
{
	kprintf("\nHAL: _irq_init()");
}

void _device_init(void)
{
	kprintf("\nHAL: _device_init()");
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
		
#if TIME_SLICE == 0
	IFSCLR(0) = 0x00000200;
#else
	IFSCLR(0) = 0x00004000;
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
	return mfc0(CP0_COUNT, 0);
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

void _soft_reset()
{
	NVMKEY = 0x0;
	NVMKEY = 0xAA996655;
	NVMKEY = 0x556699AA;

	RSWRST |= 1;
    
	/* read RSWRST register to trigger reset */
	volatile int32_t *p = (int32_t *)&RSWRST;
	*p;

	/* prevent any unwanted code execution until reset occurs*/
	while (1);  
}

void _panic(void)
{
}
