#include <hellfire.h>

#ifdef USTACK
#include <ustack.h>

void eth_isr()
{
	int32_t eth_driver;
	
	eth_driver = hf_id("ustack");
	if (eth_driver >= 0 && eth_driver < MAX_TASKS)
		krnl_tcb[eth_driver].critical = 1;
//	IRQ_MASK &= ~EXT_IRQ2_NOT;
}

void en_irqack()
{
//	IRQ_MASK |= EXT_IRQ2_NOT;
}

void en_irqconfig()
{
	/* SPI_IRQ0, active low */
//	_irq_register(EXT_IRQ2_NOT, eth_isr);
//	IRQ_MASK |= EXT_IRQ2_NOT;
}
#endif
