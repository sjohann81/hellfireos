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

#include <pic32mz.h>

#define RA				0
#define RB				1
#define RC				2
#define RD				3
#define RE				4
#define RF				5
#define RG				6
#define RH				7
#define RJ				9
#define RK				10

/* SPI read / write ports */
#define SPI_INPORT			RK
#define SPI_OUTPORT			RK

/* SPI interface - EXTIO_OUT */
#define SPI_SCK				(1 << 2)
#define SPI_MOSI			(1 << 3)
#define SPI_CS0				(1 << 1)

/* SPI interface - EXTIO_IN */
#define SPI_MISO			(1 << 4)
#define SPI_IRQ0			(1 << 5)

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
uint32_t _port_read(uint32_t port);
void _port_write(uint32_t port, uint32_t val);
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
void _soft_reset();
void _panic(void);
