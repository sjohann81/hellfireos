/* C type extensions */
typedef unsigned char			uint8_t;
typedef char				int8_t;
typedef unsigned short int		uint16_t;
typedef short int			int16_t;
typedef unsigned int			uint32_t;
typedef int				int32_t;
typedef unsigned long long		uint64_t;
typedef long long			int64_t;
typedef unsigned long			size_t;
typedef void				(*funcptr)();

/* disable interrupts, return previous int status / enable interrupts */
#define IRQ_FLAG			0x80

/* peripherals */

/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0183g/I18381.html*/
#define UART0_BASE			0x101f1000
#define UART0DR				(*(volatile uint32_t *)(UART0_BASE+0x000))
#define UART0FR				(*(volatile uint32_t *)(UART0_BASE+0x018))
#define UARTFR_TXFF			0x20
#define UARTFR_RXFF			0x10

#define TIMER0_BASE			0x101e2000
#define TIMER0_LOAD			(*(volatile uint32_t *)(TIMER0_BASE+0x00))
#define TIMER0_VALUE			(*(volatile uint32_t *)(TIMER0_BASE+0x04))
#define TIMER0_CONTROL			(*(volatile uint32_t *)(TIMER0_BASE+0x08))
#define TIMER0_INTCLR			(*(volatile uint32_t *)(TIMER0_BASE+0x0c))
#define TIMER0_RIS			(*(volatile uint32_t *)(TIMER0_BASE+0x10))
#define TIMER0_MIS			(*(volatile uint32_t *)(TIMER0_BASE+0x14))
#define TIMER0_BGLOAD			(*(volatile uint32_t *)(TIMER0_BASE+0x18))

#define TIMER3_BASE			0x101e3020
#define TIMER3_LOAD			(*(volatile uint32_t *)(TIMER3_BASE+0x00))
#define TIMER3_VALUE			(*(volatile uint32_t *)(TIMER3_BASE+0x04))
#define TIMER3_CONTROL			(*(volatile uint32_t *)(TIMER3_BASE+0x08))
#define TIMER3_INTCLR			(*(volatile uint32_t *)(TIMER3_BASE+0x0c))
#define TIMER3_RIS			(*(volatile uint32_t *)(TIMER3_BASE+0x10))
#define TIMER3_MIS			(*(volatile uint32_t *)(TIMER3_BASE+0x14))
#define TIMER3_BGLOAD			(*(volatile uint32_t *)(TIMER3_BASE+0x18))

#define TIMER_EN			0x80
#define TIMER_PERIODIC			0x40
#define TIMER_INTEN			0x20
#define TIMER_32BIT			0x02
#define TIMER_ONESHOT			0x01

/* interrupt controller */
#define NVIC_BASE			0x10140000
#define VIC_IRQSTATUS			(*(volatile uint32_t *)(NVIC_BASE+0x000))
#define VIC_FIQSTATUS			(*(volatile uint32_t *)(NVIC_BASE+0x004))
#define VIC_RAWINTR			(*(volatile uint32_t *)(NVIC_BASE+0x008))
#define VIC_INTSELECT			(*(volatile uint32_t *)(NVIC_BASE+0x00c))
#define VIC_IRQENABLE			(*(volatile uint32_t *)(NVIC_BASE+0x010))
#define VIC_IRQENCLEAR			(*(volatile uint32_t *)(NVIC_BASE+0x014))
#define VIC_IRQSOFT			(*(volatile uint32_t *)(NVIC_BASE+0x018))
#define VIC_IRQSOFTCLEAR		(*(volatile uint32_t *)(NVIC_BASE+0x01c))
#define VIC_PROTECT			(*(volatile uint32_t *)(NVIC_BASE+0x020))
#define VIC_VECTADDR			(*(volatile uint32_t *)(NVIC_BASE+0x030))
#define VIC_DEFVECTADDR			(*(volatile uint32_t *)(NVIC_BASE+0x034))

/*
source:
https://github.com/Luminger/Alice-1121-Modem/blob/master/kernel/linux/include/asm-arm/arch-versatile/platform.h
https://github.com/Luminger/Alice-1121-Modem/blob/master/kernel/linux/include/asm-arm/arch-versatile/irqs.h
*/
#define INT_WDOGINT                     0	/* Watchdog timer */
#define INT_SOFTINT                     1	/* Software interrupt */
#define INT_COMMRx                      2	/* Debug Comm Rx interrupt */
#define INT_COMMTx                      3	/* Debug Comm Tx interrupt */
#define INT_TIMERINT0_1                 4	/* Timer 0 and 1 */
#define INT_TIMERINT2_3                 5	/* Timer 2 and 3 */
#define INT_GPIOINT0                    6	/* GPIO 0 */
#define INT_GPIOINT1                    7	/* GPIO 1 */
#define INT_GPIOINT2                    8	/* GPIO 2 */
#define INT_GPIOINT3                    9	/* GPIO 3 */
#define INT_RTCINT                      10	/* Real Time Clock */
#define INT_SSPINT                      11	/* Synchronous Serial Port */
#define INT_UARTINT0                    12	/* UART 0 on development chip */
#define INT_UARTINT1                    13	/* UART 1 on development chip */
#define INT_UARTINT2                    14	/* UART 2 on development chip */
#define INT_SCIINT                      15	/* Smart Card Interface */
#define INT_CLCDINT                     16	/* CLCD controller */
#define INT_DMAINT                      17	/* DMA controller */
#define INT_PWRFAILINT                  18	/* Power failure */
#define INT_MBXINT                      19	/* Graphics processor */
#define INT_GNDINT                      20	/* Reserved */

#define INTMASK_WDOGINT                 (1 << INT_WDOGINT)
#define INTMASK_SOFTINT                 (1 << INT_SOFTINT)
#define INTMASK_COMMRx                  (1 << INT_COMMRx)
#define INTMASK_COMMTx                  (1 << INT_COMMTx)
#define INTMASK_TIMERINT0_1             (1 << INT_TIMERINT0_1)
#define INTMASK_TIMERINT2_3             (1 << INT_TIMERINT2_3)
#define INTMASK_GPIOINT0                (1 << INT_GPIOINT0)
#define INTMASK_GPIOINT1                (1 << INT_GPIOINT1)
#define INTMASK_GPIOINT2                (1 << INT_GPIOINT2)
#define INTMASK_GPIOINT3                (1 << INT_GPIOINT3)
#define INTMASK_RTCINT                  (1 << INT_RTCINT)
#define INTMASK_SSPINT                  (1 << INT_SSPINT)
#define INTMASK_UARTINT0                (1 << INT_UARTINT0)
#define INTMASK_UARTINT1                (1 << INT_UARTINT1)
#define INTMASK_UARTINT2                (1 << INT_UARTINT2)
#define INTMASK_SCIINT                  (1 << INT_SCIINT)
#define INTMASK_CLCDINT                 (1 << INT_CLCDINT)
#define INTMASK_DMAINT                  (1 << INT_DMAINT)
#define INTMASK_PWRFAILINT              (1 << INT_PWRFAILINT)
#define INTMASK_MBXINT                  (1 << INT_MBXINT)
#define INTMASK_GNDINT                  (1 << INT_GNDINT)

/* hardware dependent stuff */
#define STACK_MAGIC			0xb00bb00b
typedef uint32_t context[16];

int32_t _interrupt_set(int32_t s);
void _irq_mask_set(uint32_t mask);
uint32_t _irq_mask_clr(uint32_t mask);

/* hardware dependent C library stuff */
int32_t _context_save(context env);
void _context_restore(context env, int32_t val);
uint32_t _di(void);
void _ei(uint32_t status);
void putchar(int32_t value);
int32_t kbhit(void);
int32_t getchar(void);
void dputchar(int32_t value);

/* hardware dependent stuff */
void _ei(uint32_t status);
uint32_t _di(void);
void delay_ms(uint32_t msec);
void delay_us(uint32_t usec);
void led_set(uint16_t led, uint8_t val);
uint8_t button_get(uint16_t btn);
uint8_t switch_get(uint16_t sw);

/* hardware dependent basic kernel stuff */
void _hardware_init(void);
void _vm_init(void);
void _task_init(void);
void _sched_init(void);
void _timer_init(void);
void _irq_init(void);
void _device_init(void);
void _set_task_sp(uint16_t task, size_t stack);
size_t _get_task_sp(uint16_t task);
void _set_task_tp(uint16_t task, void (*entry)());
void *_get_task_tp(uint16_t task);
void _timer_reset(void);
void _cpu_idle(void);
uint32_t _readcounter(void);
uint64_t _read_us(void);
void _panic(void);
