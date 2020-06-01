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
//#define IRQ_FLAG			0x01

/* configure, read and write board pins */
#define _port_setup(a, opts)		*(volatile uint32_t *)(a) = (opts)
#define _port_read(a)			(*(volatile uint32_t *)(a))
#define _port_write(a, v)		*(volatile uint32_t *)(a) = (v)

#define read_csr(reg) ({ uint32_t __tmp; asm volatile ("csrr %0, " #reg : "=r"(__tmp)); __tmp; })
#define write_csr(reg, val) ({ asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#define NS16550A_UART0_CTRL_ADDR(a)	*(uint8_t*) (0x10000000 + (a))
#define NS16550A_RBR			0x00
#define NS16550A_THR      		0x00
#define NS16550A_IER      		0x01
#define NS16550A_DLL      		0x00
#define NS16550A_DLM      		0x01
#define NS16550A_FCR      		0x02
#define NS16550A_LCR      		0x03
#define NS16550A_MCR      		0x04
#define NS16550A_LSR      		0x05
#define NS16550A_MSR      		0x06
#define NS16550A_SCR      		0x07
#define NS16550A_LCR_DLAB 		0x80
#define NS16550A_LCR_8BIT 		0x03
#define NS16550A_LCR_PODD 		0x08
#define NS16550A_LSR_DA   		0x01
#define NS16550A_LSR_OE   		0x02
#define NS16550A_LSR_PE   		0x04
#define NS16550A_LSR_FE   		0x08
#define NS16550A_LSR_BI   		0x10
#define NS16550A_LSR_RE  	 	0x20
#define NS16550A_LSR_RI   		0x40
#define NS16550A_LSR_EF   		0x80

#define MTIME				(*(volatile uint64_t *)(0x0200bff8))
#define MTIMECMP			(*(volatile uint64_t *)(0x02004000))
#define MTIME_L				(*(volatile uint32_t *)(0x0200bff8))
#define MTIME_H				(*(volatile uint32_t *)(0x0200bffc))
#define MTIMECMP_L			(*(volatile uint32_t *)(0x02004000))
#define MTIMECMP_H			(*(volatile uint32_t *)(0x02004004))

/* hardware dependent stuff */
#define STACK_MAGIC			0xb00bb00b
typedef uint32_t context[20];

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
uint32_t _readcounter(void);
uint64_t _read_us(void);

uint64_t mtime_r(void);
void mtime_w(uint64_t val);
uint64_t mtimecmp_r(void);
void mtimecmp_w(uint64_t val);

void _panic(void);
