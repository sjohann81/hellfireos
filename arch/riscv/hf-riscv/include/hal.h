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

/* memory address map */
#define ADDR_ROM_BASE			0x00000000
#define ADDR_RAM_BASE			0x40000000
#define ADDR_RESERVED_BASE		0x80000000

/* peripheral addresses and irq lines */
#define PERIPHERALS_BASE		0xf0000000
#define IRQ_VECTOR			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x000))
#define IRQ_CAUSE			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x010))
#define IRQ_MASK			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x020))
#define IRQ_STATUS			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x030))
#define IRQ_EPC				(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x040))
#define COUNTER				(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x050))
#define COMPARE				(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x060))
#define COMPARE2			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x070))
#define EXTIO_IN			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x080))
#define EXTIO_OUT			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x090))
#define EXTIO_DIR			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x0a0))
#define DEBUG_ADDR			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x0d0))
#define UART				(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x0e0))
#define UART_DIVISOR			(*(volatile uint32_t *)(PERIPHERALS_BASE + 0x0f0))

#define IRQ_COUNTER			0x0001
#define IRQ_COUNTER_NOT			0x0002
#define IRQ_COUNTER2			0x0004
#define IRQ_COUNTER2_NOT		0x0008
#define IRQ_COMPARE			0x0010
#define IRQ_COMPARE2			0x0020
#define IRQ_UART_READ_AVAILABLE		0x0040
#define IRQ_UART_WRITE_AVAILABLE	0x0080

#define EXT_IRQ0			0x0100
#define EXT_IRQ1			0x0200
#define EXT_IRQ2			0x0400
#define EXT_IRQ3			0x0800
#define EXT_IRQ4			0x1000
#define EXT_IRQ5			0x2000
#define EXT_IRQ6			0x4000
#define EXT_IRQ7			0x8000

#define STACK_MAGIC			0xb00bb00b
typedef uint32_t context[20];

/* hardware dependent stuff */
int32_t _interrupt_set(int32_t s);
void _restoreexec(context env, int32_t val, int32_t ctask);

/* hardware dependent C library stuff */
int32_t setjmp(context env);
void longjmp(context env, int32_t val);
void putchar(int32_t value);
int32_t kbhit(void);
int32_t getchar(void);
void dputchar(int32_t value);

/* hardware dependent stuff */
void delay_ms(uint32_t msec);
void delay_us(uint32_t usec);

/* hardware dependent basic kernel stuff */
void _hardware_init(void);
void _vm_init(void);
void _task_init(void);
void _sched_init(void);
void _timer_init(void);
void _irq_init(void);
void _device_init(void);
void _set_task_sp(uint16_t task, uint32_t stack);
uint32_t _get_task_sp(uint16_t task);
void _set_task_tp(uint16_t task, void (*entry)());
void *_get_task_tp(uint16_t task);
void _timer_reset(void);
void _cpu_idle(void);
uint32_t _readcounter(void);
uint64_t _read_us(void);

void _restoreexec(context env, int32_t val, int32_t ctask);
void _irq_handler(uint32_t cause, uint32_t *stack);
void _except_handler(uint32_t epc, uint32_t opcode);
void _irq_register(uint32_t mask, funcptr ptr);
