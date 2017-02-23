#include <hellfire.h>

static funcptr isr[7][32] = {[0 ... 6][0 ... 31] = NULL};

/*
interrupt management routines
*/
void _irq_register(uint32_t port, uint32_t mask, funcptr ptr)
{
	int32_t i;

	for (i = 0; i < 32; ++i)
		if (mask & (1 << i))
			isr[port][i] = ptr;
}

void _irq_handler(uint32_t status, uint32_t cause)
{
	int32_t i, p;
	uint32_t irq;

	krnl_pcb.interrupts++;	
	for (p = 0; p < 7; p++){
		i = 0;
		irq = IFS(p);
		do {
			if (irq & 0x1){
				if(isr[p][i]){
					isr[p][i]();
				}
			}
			irq >>= 1;
			++i;
		} while(irq);
	}
	
	if (!(PORTB & (1 << 12))){
		_di();
		kprintf("\nRESET (SW1) pressed - performing soft reset.\n");
		delay_ms(10);
		_soft_reset();
	}
}

void _irq_mask_set(uint32_t port, uint32_t mask)
{
	IECSET(port) = mask;
}

void _irq_mask_clr(uint32_t port, uint32_t mask)
{
	IECCLR(port) = mask;
}

void _exception_handler(uint32_t epc, uint32_t opcode)
{
}
