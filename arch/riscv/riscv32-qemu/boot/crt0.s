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
	addi	sp, sp, -80
	sw	ra, 0(sp)
	sw	t0, 4(sp)
	sw	t1, 8(sp)
	sw	t2, 12(sp)
	sw	a0, 16(sp)
	sw	a1, 20(sp)
	sw	a2, 24(sp)
	sw	a3, 28(sp)
	sw	a4, 32(sp)
	sw	a5, 36(sp)
	sw	a6, 40(sp)
	sw	a7, 44(sp)
	sw	t3, 48(sp)
	sw	t4, 52(sp)
	sw	t5, 56(sp)
	sw	t6, 60(sp)

	csrr    a0, mcause
	csrr    a1, mepc
	sw	a0, 64(sp)
	sw	a1, 68(sp)
	
	jal	ra, _irq_handler

	lw	a1, 68(sp)
	lw	a0, 64(sp)
	csrw	mepc, a1
	csrw	mcause, a0

	lw	ra, 0(sp)
	lw	t0, 4(sp)
	lw	t1, 8(sp)
	lw	t2, 12(sp)
	lw	a0, 16(sp)
	lw	a1, 20(sp)
	lw	a2, 24(sp)
	lw	a3, 28(sp)
	lw	a4, 32(sp)
	lw	a5, 36(sp)
	lw	a6, 40(sp)
	lw	a7, 44(sp)
	lw	t3, 48(sp)
	lw	t4, 52(sp)
	lw	t5, 56(sp)
	lw	t6, 60(sp)
	addi	sp, sp, 80
	mret

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
	sw    s10,40(a0)
	sw    s11,44(a0)
	sw    gp, 48(a0)
	sw    tp, 52(a0)
	sw    sp, 56(a0)
	sw    ra, 60(a0)
	ori   a0, zero, 0
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
	lw    s10,40(a0)
	lw    s11,44(a0)
	lw    gp, 48(a0)
	lw    tp, 52(a0)
	lw    sp, 56(a0)
	lw    ra, 60(a0)
	ori   a0, a1, 0
	csrs  mstatus, 8
	ret
