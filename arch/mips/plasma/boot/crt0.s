	.text
	.align 2

	.global _entry
	.ent	_entry
_entry:
	.set noreorder
	.set noat

	la	$a1, _bss_start
	la	$a0, _end
	la	$gp, _gp
	la	$sp, _stack

$BSS_CLEAR:
	# clear the .bss
	sw	$zero, 0($a1)
	slt	$v1, $a1, $a0
	addiu	$a1, $a1, 4
	bnez	$v1, $BSS_CLEAR
	nop

	# jump to main
	jal	main
	nop

	li	$k0, 0xe0000000		# this will interrupt the simulation (assertion)
	sw	$zero, 0($k0)

$L1:
	beq	$zero, $zero, $L1
	nop
.end _entry


# the non-vectored interrupt service routine
	.org 0x100
	.global _isr
	.ent _isr
_isr:
	.set noreorder
	.set noat

	addi  $sp, $sp, -104
	sw    $at, 16($sp)
	sw    $v0, 20($sp)
	sw    $v1, 24($sp)
	sw    $a0, 28($sp)
	sw    $a1, 32($sp)
	sw    $a2, 36($sp)
	sw    $a3, 40($sp)
	sw    $t0, 44($sp)
	sw    $t1, 48($sp)
	sw    $t2, 52($sp)
	sw    $t3, 56($sp)
	sw    $t4, 60($sp)
	sw    $t5, 64($sp)
	sw    $t6, 68($sp)
	sw    $t7, 72($sp)
	sw    $t8, 76($sp)
	sw    $t9, 80($sp)
	sw    $ra, 84($sp)
	mfc0  $k0, $14        #C0_EPC=14 (Exception PC)
	addi  $k0, $k0, -4    #Backup one opcode
	sw    $k0, 88($sp)
	mfhi  $k1
	sw    $k1, 92($sp)
	mflo  $k1
	sw    $k1, 96($sp)

	lui   $a1,  0x2000
	lw    $a0,  0x20($a1)   #IRQ_STATUS
	lw    $a2,  0x10($a1)   #IRQ_MASK
	and   $a0,  $a0, $a2
	jal   _irq_handler
	addiu	$a1,  $sp, 0

	#Restore all temporary registers
	lw    $at, 16($sp)
	lw    $v0, 20($sp)
	lw    $v1, 24($sp)
	lw    $a0, 28($sp)
	lw    $a1, 32($sp)
	lw    $a2, 36($sp)
	lw    $a3, 40($sp)
	lw    $t0, 44($sp)
	lw    $t1, 48($sp)
	lw    $t2, 52($sp)
	lw    $t3, 56($sp)
	lw    $t4, 60($sp)
	lw    $t5, 64($sp)
	lw    $t6, 68($sp)
	lw    $t7, 72($sp)
	lw    $t8, 76($sp)
	lw    $t9, 80($sp)
	lw    $ra, 84($sp)
	lw    $k0, 88($sp)
	lw    $k1, 92($sp)
	mthi  $k1
	lw    $k1, 96($sp)
	mtlo  $k1
	addi  $sp, $sp, 104

isr_return:
	ori   $k1, $zero, 0x1    #re-enable interrupts
	jr    $k0
	mtc0  $k1, $12

.end _isr

	.set at
	.align 2

	.global _interrupt_set
	.ent _interrupt_set
_interrupt_set:
	.set noreorder

	mfc0  $v0, $12
	mtc0  $a0, $12            #STATUS=1; enable interrupts
	jr    $ra
	mtc0  $a0, $12            #STATUS=1; enable interrupts

	.set reorder
.end _interrupt_set

	.set at
	.align 2

	.global _interrupt_init
	.ent _interrupt_init
_interrupt_init:
	.set noreorder

	#Patch interrupt vector
	la    $a1, _irq_patch
	lw    $a2, 0($a1)
	sw    $a2, 0x3c($zero)
	lw    $a2, 4($a1)
	sw    $a2, 0x40($zero)
	lw    $a2, 8($a1)
	sw    $a2, 0x44($zero)
	lw    $a2, 12($a1)
	sw    $a2, 0x48($zero)

	jr    $ra
	nop

_irq_patch:
	la    $k0, _isr
	jr    $k0
	nop

	.set reorder
.end _interrupt_init


	.global   _context_save
	.ent     _context_save
_context_save:
	.set noreorder

	sw    $s0, 0($a0)
	sw    $s1, 4($a0)
	sw    $s2, 8($a0)
	sw    $s3, 12($a0)
	sw    $s4, 16($a0)
	sw    $s5, 20($a0)
	sw    $s6, 24($a0)
	sw    $s7, 28($a0)
	sw    $fp, 32($a0)
	sw    $gp, 36($a0)
	sw    $sp, 40($a0)
	sw    $ra, 44($a0)

	jr    $ra
	ori   $v0,  $zero, 0

	.set reorder
.end _context_save


	.global   _context_restore
	.ent     _context_restore
_context_restore:
	.set noreorder

	lw    $s0, 0($a0)
	lw    $s1, 4($a0)
	lw    $s2, 8($a0)
	lw    $s3, 12($a0)
	lw    $s4, 16($a0)
	lw    $s5, 20($a0)
	lw    $s6, 24($a0)
	lw    $s7, 28($a0)
	lw    $fp, 32($a0)
	lw    $gp, 36($a0)
	lw    $sp, 40($a0)
	lw    $ra, 44($a0)

	ori   $k1, $zero, 0x1    # enable interrupts
	mtc0  $k1, $12

	jr    $ra
	ori   $v0,  $a1, 0

	.set reorder
.end _context_restore
