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

	# setup trap vector
	la	t0, _isr
	csrw	mtvec, t0

	# jump to main
	jal	ra, main

	j	_panic
L1:
	wfi
	beq	zero, zero, L1

# interrupt / exception service routine
	.org 0x100
	.global _isr
_isr:
	addi	sp, sp, -160
	sd	ra, 0(sp)
	sd	t0, 8(sp)
	sd	t1, 16(sp)
	sd	t2, 24(sp)
	sd	a0, 32(sp)
	sd	a1, 40(sp)
	sd	a2, 48(sp)
	sd	a3, 56(sp)
	sd	a4, 64(sp)
	sd	a5, 72(sp)
	sd	a6, 80(sp)
	sd	a7, 88(sp)
	sd	t3, 96(sp)
	sd	t4, 104(sp)
	sd	t5, 112(sp)
	sd	t6, 120(sp)

	csrr    a0, mcause
	csrr    a1, mepc
	sd	a0, 128(sp)
	sd	a1, 136(sp)
	
	jal	ra, _irq_handler

	ld	a1, 136(sp)
	ld	a0, 128(sp)
	csrw	mepc, a1
	csrw	mcause, a0

	ld	ra, 0(sp)
	ld	t0, 8(sp)
	ld	t1, 16(sp)
	ld	t2, 24(sp)
	ld	a0, 32(sp)
	ld	a1, 40(sp)
	ld	a2, 48(sp)
	ld	a3, 56(sp)
	ld	a4, 64(sp)
	ld	a5, 72(sp)
	ld	a6, 80(sp)
	ld	a7, 88(sp)
	ld	t3, 96(sp)
	ld	t4, 104(sp)
	ld	t5, 112(sp)
	ld	t6, 120(sp)
	addi	sp, sp, 160
	mret

	.global   _context_save
_context_save:
	sd    s0, 0(a0)
	sd    s1, 8(a0)
	sd    s2, 16(a0)
	sd    s3, 24(a0)
	sd    s4, 32(a0)
	sd    s5, 40(a0)
	sd    s6, 48(a0)
	sd    s7, 56(a0)
	sd    s8, 64(a0)
	sd    s9, 72(a0)
	sd    s10,80(a0)
	sd    s11,88(a0)
	sd    gp, 96(a0)
	sd    tp, 104(a0)
	sd    sp, 112(a0)
	sd    ra, 120(a0)
	ori   a0, zero, 0
	ret

	.global   _context_restore
_context_restore:
	ld    s0, 0(a0)
	ld    s1, 8(a0)
	ld    s2, 16(a0)
	ld    s3, 24(a0)
	ld    s4, 32(a0)
	ld    s5, 40(a0)
	ld    s6, 48(a0)
	ld    s7, 56(a0)
	ld    s8, 64(a0)
	ld    s9, 72(a0)
	ld    s10,80(a0)
	ld    s11,88(a0)
	ld    gp, 96(a0)
	ld    tp, 104(a0)
	ld    sp, 112(a0)
	ld    ra, 120(a0)
	ori   a0, a1, 0
	csrs  mstatus, 8
	ret
