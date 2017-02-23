#include <hellfire.h>

static funcptr isr[32] = {[0 ... 31] = NULL};

/*
interrupt management routines
*/
void _irq_register(uint32_t mask, funcptr ptr)
{
	int32_t i;

	for (i = 0; i < 32; ++i)
		if (mask & (1 << i))
			isr[i] = ptr;
}

void _irq_handler(uint32_t cause, uint32_t *stack)
{
	int32_t i = 0;

#if KERNEL_LOG >= 1
	dprintf("irq%x %d ", cause, (uint32_t)_read_us());
#endif
	krnl_pcb.interrupts++;
	do {
		if (cause & 0x1){
			if(isr[i]){
				isr[i](stack);
			}
		}
		cause >>= 1;
		++i;
	} while(cause);
}

void _irq_mask_set(uint32_t mask)
{
	uint32_t m;

	m = IRQ_MASK;
	m |= mask;
	IRQ_MASK = m;
}

uint32_t _irq_mask_clr(uint32_t mask)
{
	uint32_t m, r;
	
	m = IRQ_MASK;
	r = m;
	m &= ~mask;
	IRQ_MASK = m;
	
	return r;
}

void _exception_handler(uint32_t epc, uint32_t opcode)
{
}
