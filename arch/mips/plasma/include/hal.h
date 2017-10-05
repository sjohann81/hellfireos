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
#define _di()				_interrupt_set(0)
#define _ei(S)				_interrupt_set(S)
#define IRQ_FLAG			0x01

/* configure, read and write board pins */
#define _port_setup(a, opts)		*(volatile uint32_t *)(a) = (opts)
#define _port_read(a)			(*(volatile uint32_t *)(a))
#define _port_write(a, v)		*(volatile uint32_t *)(a) = (v)

#define MemoryRead(A)			(*(volatile unsigned int*)(A))
#define MemoryWrite(A,V)		*(volatile unsigned int*)(A)=(V)

/* peripheral addresses and irq lines */
#define UART_WRITE			0x20000000
#define UART_READ			0x20000000
#define IRQ_MASK			0x20000010
#define IRQ_STATUS			0x20000020
#define GPIO0_OUT			0x20000030
#define GPIO1_OUT			0x20000040
#define GPIOA_IN			0x20000050
#define COUNTER_REG			0x20000060

#define FREQUENCY_REG			0x200000A0	/* simulator only */
#define TICK_TIME_REG			0x200000B0	/* simulator only */
#define OUT_FACILITY			0x200000D0	/* not implemented on hw, but yeah on sim */
#define LOG_FACILITY			0x200000E0	/* simulator only */

#define IRQ_UART_READ_AVAILABLE		0x01
#define IRQ_UART_WRITE_AVAILABLE	0x02
#define IRQ_COUNTER18_NOT		0x04
#define IRQ_COUNTER18			0x08
#define IRQ_GPIO30_NOT			0x10
#define IRQ_GPIO31_NOT			0x20
#define IRQ_GPIO30			0x40
#define IRQ_GPIO31			0x80
#define IRQ_NOC_READ			0x100

#define TICK_TIME_PERIOD (1<<TICK_TIME) / (CPU_SPEED / 1000000)


#define STACK_MAGIC			0xb00bb00b
typedef uint32_t context[20];

/* hardware dependent stuff */
int32_t _interrupt_set(int32_t s);

/* hardware dependent C library stuff */
int32_t _context_save(context env);
void _context_restore(context env, int32_t val);
void putchar(int32_t value);
int32_t kbhit(void);
int32_t getchar(void);
void dputchar(int32_t value);

/* hardware dependent stuff */
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
