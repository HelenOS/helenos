#
# Copyright (c) 2003-2004 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#include <abi/asmtool.h>
#include <arch/asm/regname.h>
#include <arch/mm/page.h>
#include <arch/asm/boot.h>
#include <arch/stack.h>
#include <arch/istate_struct.h>

.text

.set noat
.set noreorder

/*
 * Which status bits are thread-local:
 * KSU(UM), EXL, ERL, IE
 */
#define REG_SAVE_MASK 0x1f

/*
 * The fake ABI prologue is never executed and may not be part of the
 * procedure's body. Instead, it should be immediately preceding the procedure's
 * body. Its only purpose is to trick the stack trace walker into thinking that
 * the exception is more or less just a normal function call.
 */
.macro FAKE_ABI_PROLOGUE
	sub $sp, ISTATE_SIZE
	sw $ra, ISTATE_OFFSET_EPC($sp)
.endm

/*
 * Save registers to space defined by \r
 * We will change status: Disable ERL, EXL, UM, IE
 * These changes will be automatically reversed in REGISTER_LOAD
 * %sp is NOT saved as part of these registers
 */
.macro REGISTERS_STORE_AND_EXC_RESET r
	sw $at, ISTATE_OFFSET_AT(\r)
	sw $v0, ISTATE_OFFSET_V0(\r)
	sw $v1, ISTATE_OFFSET_V1(\r)
	sw $a0, ISTATE_OFFSET_A0(\r)
	sw $a1, ISTATE_OFFSET_A1(\r)
	sw $a2, ISTATE_OFFSET_A2(\r)
	sw $a3, ISTATE_OFFSET_A3(\r)
	sw $t0, ISTATE_OFFSET_T0(\r)
	sw $t1, ISTATE_OFFSET_T1(\r)
	sw $t2, ISTATE_OFFSET_T2(\r)
	sw $t3, ISTATE_OFFSET_T3(\r)
	sw $t4, ISTATE_OFFSET_T4(\r)
	sw $t5, ISTATE_OFFSET_T5(\r)
	sw $t6, ISTATE_OFFSET_T6(\r)
	sw $t7, ISTATE_OFFSET_T7(\r)
	sw $t8, ISTATE_OFFSET_T8(\r)
	sw $t9, ISTATE_OFFSET_T9(\r)
	sw $s0, ISTATE_OFFSET_S0(\r)
	sw $s1, ISTATE_OFFSET_S1(\r)
	sw $s2, ISTATE_OFFSET_S2(\r)
	sw $s3, ISTATE_OFFSET_S3(\r)
	sw $s4, ISTATE_OFFSET_S4(\r)
	sw $s5, ISTATE_OFFSET_S5(\r)
	sw $s6, ISTATE_OFFSET_S6(\r)
	sw $s7, ISTATE_OFFSET_S7(\r)
	sw $s8, ISTATE_OFFSET_S8(\r)

	mflo $at
	sw $at, ISTATE_OFFSET_LO(\r)
	mfhi $at
	sw $at, ISTATE_OFFSET_HI(\r)

	sw $gp, ISTATE_OFFSET_GP(\r)
	sw $ra, ISTATE_OFFSET_RA(\r)
	sw $k0, ISTATE_OFFSET_KT0(\r)
	sw $k1, ISTATE_OFFSET_KT1(\r)

	mfc0 $t0, $status
	mfc0 $t1, $epc

	/* save only KSU, EXL, ERL, IE */
	and $t2, $t0, REG_SAVE_MASK

	/* clear KSU, EXL, ERL, IE */
	li $t3, ~(REG_SAVE_MASK)
	and $t0, $t0, $t3

	sw $t2, ISTATE_OFFSET_STATUS(\r)
	sw $t1, ISTATE_OFFSET_EPC(\r)
	mtc0 $t0, $status
.endm

.macro REGISTERS_LOAD r
	/*
	 * Update only UM, EXR, IE from status, the rest
	 * is controlled by OS and not bound to task.
	 */
	mfc0 $t0, $status
	lw $t1, ISTATE_OFFSET_STATUS(\r)

	/* mask UM, EXL, ERL, IE */
	li $t2, ~REG_SAVE_MASK
	and $t0, $t0, $t2

	/* copy UM, EXL, ERL, IE from saved status */
	or $t0, $t0, $t1
	mtc0 $t0, $status

	lw $v0, ISTATE_OFFSET_V0(\r)
	lw $v1, ISTATE_OFFSET_V1(\r)
	lw $a0, ISTATE_OFFSET_A0(\r)
	lw $a1, ISTATE_OFFSET_A1(\r)
	lw $a2, ISTATE_OFFSET_A2(\r)
	lw $a3, ISTATE_OFFSET_A3(\r)
	lw $t0, ISTATE_OFFSET_T0(\r)
	lw $t1, ISTATE_OFFSET_T1(\r)
	lw $t2, ISTATE_OFFSET_T2(\r)
	lw $t3, ISTATE_OFFSET_T3(\r)
	lw $t4, ISTATE_OFFSET_T4(\r)
	lw $t5, ISTATE_OFFSET_T5(\r)
	lw $t6, ISTATE_OFFSET_T6(\r)
	lw $t7, ISTATE_OFFSET_T7(\r)
	lw $t8, ISTATE_OFFSET_T8(\r)
	lw $t9, ISTATE_OFFSET_T9(\r)

	lw $gp, ISTATE_OFFSET_GP(\r)
	lw $ra, ISTATE_OFFSET_RA(\r)
	lw $k1, ISTATE_OFFSET_KT1(\r)

	lw $at, ISTATE_OFFSET_LO(\r)
	mtlo $at
	lw $at, ISTATE_OFFSET_HI(\r)
	mthi $at

	lw $at, ISTATE_OFFSET_EPC(\r)
	mtc0 $at, $epc

	lw $at, ISTATE_OFFSET_AT(\r)
	lw $sp, ISTATE_OFFSET_SP(\r)
.endm

/*
 * Move kernel stack pointer address to register $k0.
 * If we are in user mode, load the appropriate stack address.
 */
.macro KERNEL_STACK_TO_K0
	/* if we are in user mode */
	mfc0 $k0, $status
	andi $k0, 0x10

	beq $k0, $0, 1f
	move $k0, $sp

	/* move $k0 pointer to kernel stack */
	la $k0, supervisor_sp

	/* move $k0 (supervisor_sp) */
	lw $k0, ($k0)

	1:
.endm

.org 0x0
SYMBOL(kernel_image_start)
	/* load temporary stack */
	lui $sp, %hi(end_stack)
	ori $sp, $sp, %lo(end_stack)

	/* not sure about this, but might be needed for PIC code */
	lui $gp, 0x8000

	/* $a1 contains physical address of bootinfo_t */
	jal mips32_pre_main
	addiu $sp, -ABI_STACK_FRAME

	j main_bsp
	nop

.space TEMP_STACK_SIZE
end_stack:

SYMBOL(tlb_refill_entry)
	j tlb_refill_handler
	nop

SYMBOL(cache_error_entry)
	j cache_error_handler
	nop

SYMBOL(exception_entry)
	j exception_handler
	nop

	FAKE_ABI_PROLOGUE
exception_handler:
	KERNEL_STACK_TO_K0

	sub $k0, ISTATE_SIZE
	sw $sp, ISTATE_OFFSET_SP($k0)
	move $sp, $k0

	mfc0 $k0, $cause

	sra $k0, $k0, 0x2    /* cp0_exc_cause() part 1 */
	andi $k0, $k0, 0x1f  /* cp0_exc_cause() part 2 */
	sub $k0, 8           /* 8 = SYSCALL */

	beqz $k0, syscall_shortcut
	add $k0, 8           /* revert $k0 back to correct exc number */

	REGISTERS_STORE_AND_EXC_RESET $sp

	move $a1, $sp
	move $a0, $k0
	jal exc_dispatch     /* exc_dispatch(excno, register_space) */
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME

	REGISTERS_LOAD $sp
	/* the $sp is automatically restored to former value */
	eret

/** Syscall entry
 *
 * Registers:
 *
 * @param $v0 Syscall number.
 * @param $a0 1st argument.
 * @param $a1 2nd argument.
 * @param $a2 3rd argument.
 * @param $a3 4th argument.
 * @param $t0 5th argument.
 * @param $t1 6th argument.
 *
 * @return The return value will be stored in $v0.
 *
 */
syscall_shortcut:
	mfc0 $t3, $epc
	mfc0 $t2, $status
	sw $t3, ISTATE_OFFSET_EPC($sp)  /* save EPC */

	and $t4, $t2, REG_SAVE_MASK  /* save only KSU, EXL, ERL, IE */
	li $t5, ~(0x1f)
	and $t2, $t2, $t5  /* clear KSU, EXL, ERL */
	ori $t2, $t2, 0x1  /* set IE */

	sw $t4, ISTATE_OFFSET_STATUS($sp)
	mtc0 $t2, $status

	/*
	 * Call the higher level system call handler.
	 *
	 */
	sw $t0, ISTATE_OFFSET_T0($sp)  /* save the 5th argument on the stack */
	sw $t1, ISTATE_OFFSET_T1($sp)  /* save the 6th argument on the stack */

	jal syscall_handler
	sw $v0, ISTATE_OFFSET_V0($sp)  /* save the syscall number on the stack */

	/* restore status */
	mfc0 $t2, $status
	lw $t3, ISTATE_OFFSET_STATUS($sp)

	/*
	 * Change back to EXL = 1 (from last exception), otherwise
	 * an interrupt could rewrite the CP0 - EPC.
	 *
	 */
	li $t4, ~REG_SAVE_MASK  /* mask UM, EXL, ERL, IE */
	and $t2, $t2, $t4
	or $t2, $t2, $t3  /* copy saved UM, EXL, ERL, IE */
	mtc0 $t2, $status

	/* restore epc + 4 */
	lw $t2, ISTATE_OFFSET_EPC($sp)
	addi $t2, $t2, 4
	mtc0 $t2, $epc

	lw $sp, ISTATE_OFFSET_SP($sp)  /* restore $sp */
	eret

	FAKE_ABI_PROLOGUE
tlb_refill_handler:
	KERNEL_STACK_TO_K0
	sub $k0, ISTATE_SIZE
	REGISTERS_STORE_AND_EXC_RESET $k0
	sw $sp, ISTATE_OFFSET_SP($k0)
	move $sp, $k0

	move $a0, $sp
	jal tlb_refill
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME

	REGISTERS_LOAD $sp
	eret

	FAKE_ABI_PROLOGUE
cache_error_handler:
	KERNEL_STACK_TO_K0
	sub $k0, ISTATE_SIZE
	REGISTERS_STORE_AND_EXC_RESET $k0
	sw $sp, ISTATE_OFFSET_SP($k0)
	move $sp, $k0

	move $a0, $sp
	jal cache_error
	addiu $sp, -ABI_STACK_FRAME
	addiu $sp, ABI_STACK_FRAME

	REGISTERS_LOAD $sp
	eret

FUNCTION_BEGIN(userspace_asm)
	move $sp, $a0
	xor $a0, $a0, $a0  /* $a0 is defined to hold pcb_ptr, set it to 0 */
	xor $fp, $fp, $fp  // FIXME: wipe all userspace-accessible registers
	xor $ra, $ra, $ra
	eret
FUNCTION_END(userspace_asm)
