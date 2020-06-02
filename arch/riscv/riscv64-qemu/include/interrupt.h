void _irq_handler(uint64_t cause, uint64_t *stack);
uint32_t _exception_handler(uint64_t service, uint64_t value, uint64_t epc, uint64_t opcode);
