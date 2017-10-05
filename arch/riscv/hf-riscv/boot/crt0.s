	.text
	.align 2

	.global _entry
_entry:
	la	a3, _bss_start
	la	a2, _end
	la	gp, _gp
	la	sp, _stack
	la	tp, _end + 63
	and	tp, tp, -64

BSS_CLEAR:
	# clear the .bss
	sw	zero, 0(a3)
	addi	a3, a3, 4
	blt	a3, a2, BSS_CLEAR

	# configure IRQ_VECTOR
	la	s11, _isr
	li	s10, 0xf0000000
	sw	s11, 0(s10)
	
	# jump to main
	jal	ra, main

	li	s10, 0xe0000000		# this will interrupt the simulation (assertion)
	sw	zero, 0(s10)

L1:
	beq	zero, zero, L1

# interrupt / exception service routine
	.org 0x100
	.global _isr
_isr:
	nop				# this must be a NOP
	addi	sp, sp, -116
	sw	ra, 0(sp)
	sw	gp, 4(sp)
	sw	tp, 8(sp)
	sw	t0, 12(sp)
	sw	t1, 16(sp)
	sw	t2, 20(sp)
	sw	s0, 24(sp)
	sw	s1, 28(sp)
	sw	a0, 32(sp)
	sw	a1, 36(sp)
	sw	a2, 40(sp)
	sw	a3, 44(sp)
	sw	a4, 48(sp)
	sw	a5, 52(sp)
	sw	a6, 56(sp)
	sw	a7, 60(sp)
	sw	s2, 64(sp)
	sw	s3, 68(sp)
	sw	s4, 72(sp)
	sw	s5, 76(sp)
	sw	s6, 80(sp)
	sw	s7, 84(sp)
	sw	s8, 88(sp)
	sw	s9, 92(sp)
	sw	t3, 96(sp)
	sw	t4, 100(sp)
	sw	t5, 104(sp)
	sw	t6, 108(sp)
	li	s10, 0xf0000040		# read IRQ_EPC
	lw	s10, 0(s10)
	addi	s10, s10, -4		# rollback, last opcode (at EPC) was not commited
	sw	s10, 112(sp)
	lui	a1, 0xf0000
	lw	a0, 0x10(a1)		# read IRQ_CAUSE
	lw	a2, 0x20(a1)		# read IRQ_MASK
	and	a0, a0, a2		# pass CAUSE and MASK and the stack pointer to the C handler
	addi	a1, sp, 0
	beq	a0, zero, _exception	# it's an exception, not an interrupt
	jal	ra, _irq_handler	# jump to C handler
_restore:	
	lw	ra, 0(sp)
	lw	gp, 4(sp)
	lw	tp, 8(sp)
	lw	t0, 12(sp)
	lw	t1, 16(sp)
	lw	t2, 20(sp)
	lw	s0, 24(sp)
	lw	s1, 28(sp)
	lw	a0, 32(sp)
	lw	a1, 36(sp)
	lw	a2, 40(sp)
	lw	a3, 44(sp)
	lw	a4, 48(sp)
	lw	a5, 52(sp)
	lw	a6, 56(sp)
	lw	a7, 60(sp)
	lw	s2, 64(sp)
	lw	s3, 68(sp)
	lw	s4, 72(sp)
	lw	s5, 76(sp)
	lw	s6, 80(sp)
	lw	s7, 84(sp)
	lw	s8, 88(sp)
	lw	s9, 92(sp)
	lw	t3, 96(sp)
	lw	t4, 100(sp)
	lw	t5, 104(sp)
	lw	t6, 108(sp)
	addi	sp, sp, 116
	ori	s11, zero, 0x1
	li	s10, 0xf0000030
	sw	s11, 0(s10)		# enable interrupts after a few cycles
	lw	s10, -4(sp)
	jalr	zero, s10		# context restored, continue
_exception:
	addi	s10, s10, -4		# s10 is IRQ_EPC-4, actual EPC is IRQ_EPC-8
	lw	s11, 0(s10)		# read opcode
	addi	a0, s10, 0
	addi	a1, s11, 0
	jal	ra, _exception_handler	# TODO: set rd reg on some exceptions
	jal	zero, _restore

	.global _interrupt_set
_interrupt_set:
	li	a1, 0xf0000030
	lw	a2, 0(a1)
	sw	a0, 0(a1)
	addi	a0, a2, 0
	ret

	.global   _context_save
_context_save:
	sw    s0, 0(a0)
	sw    s1, 4(a0)
	sw    s2, 8(a0)
	sw    s3, 12(a0)
	sw    s4, 16(a0)
	sw    s5, 20(a0)
	sw    s6, 24(a0)
	sw    s7, 28(a0)
	sw    s8, 32(a0)
	sw    s9, 36(a0)
#	sw    gp, 40(a0)
#	sw    tp, 44(a0)
	sw    sp, 48(a0)
	sw    ra, 52(a0)
	ori  a0, zero, 0
	ret

	.global   _context_restore
_context_restore:
	lw    s0, 0(a0)
	lw    s1, 4(a0)
	lw    s2, 8(a0)
	lw    s3, 12(a0)
	lw    s4, 16(a0)
	lw    s5, 20(a0)
	lw    s6, 24(a0)
	lw    s7, 28(a0)
	lw    s8, 32(a0)
	lw    s9, 36(a0)
#	lw    gp, 40(a0)
#	lw    tp, 44(a0)
	lw    sp, 48(a0)
	lw    ra, 52(a0)

	ori   s11, zero, 0x1
	li    s10, 0xf0000030

	ori  a0, a1, 0
	sw    s11, 0(s10)		# enable interrupts
	ret
