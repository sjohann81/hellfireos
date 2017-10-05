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

#define IRQ_COUNTER			0x00000001
#define IRQ_COUNTER_NOT			0x00000002
#define IRQ_COUNTER2			0x00000004
#define IRQ_COUNTER2_NOT		0x00000008
#define IRQ_COMPARE			0x00000010
#define IRQ_COMPARE2			0x00000020
#define IRQ_UART_READ_AVAILABLE		0x00000040
#define IRQ_UART_WRITE_AVAILABLE	0x00000080

#define EXT_IRQ0			0x00010000
#define EXT_IRQ1			0x00020000
#define EXT_IRQ2			0x00040000
#define EXT_IRQ3			0x00080000
#define EXT_IRQ4			0x00100000
#define EXT_IRQ5			0x00200000
#define EXT_IRQ6			0x00400000
#define EXT_IRQ7			0x00800000
#define EXT_IRQ0_NOT			0x01000000
#define EXT_IRQ1_NOT			0x02000000
#define EXT_IRQ2_NOT			0x04000000
#define EXT_IRQ3_NOT			0x08000000
#define EXT_IRQ4_NOT			0x10000000
#define EXT_IRQ5_NOT			0x20000000
#define EXT_IRQ6_NOT			0x40000000
#define EXT_IRQ7_NOT			0x80000000

/* SPI read / write ports */
#define SPI_INPORT			0xf0000080
#define SPI_OUTPORT			0xf0000090

/* SPI interface - EXTIO_OUT */
#define SPI_SCK				0x01
#define SPI_MOSI			0x02
#define SPI_CS0				0x04
#define SPI_CS1				0x08
#define SPI_CS2				0x10
#define SPI_CS3				0x20

/* SPI interface - EXTIO_IN */
#define SPI_MISO			0x02
#define SPI_IRQ0			0x04
#define SPI_IRQ1			0x08
#define SPI_IRQ2			0x10
#define SPI_IRQ3			0x20

/* hardware dependent stuff */
#define STACK_MAGIC			0xb00bb00b
typedef uint32_t context[20];

int32_t _interrupt_set(int32_t s);
void _irq_mask_set(uint32_t mask);
uint32_t _irq_mask_clr(uint32_t mask);

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
