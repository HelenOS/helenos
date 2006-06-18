#
# Copyright (C) 2003-2004 Jakub Jermar
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

#include <arch/asm/regname.h>
#include <arch/mm/page.h>
#include <arch/asm/boot.h>
#include <arch/context_offset.h>
	
.text

.set noat
.set noreorder
.set nomacro

.global kernel_image_start
.global tlb_refill_entry
.global cache_error_entry
.global exception_entry
.global userspace_asm

# Which status bits should are thread-local
#define REG_SAVE_MASK 0x1f # KSU(UM), EXL, ERL, IE
	
# Save registers to space defined by \r
# We will change status: Disable ERL,EXL,UM,IE
# These changes will be automatically reversed in REGISTER_LOAD
# SP is NOT saved as part of these registers
.macro REGISTERS_STORE_AND_EXC_RESET r
	sw $at,EOFFSET_AT(\r)
	sw $v0,EOFFSET_V0(\r)
	sw $v1,EOFFSET_V1(\r)
	sw $a0,EOFFSET_A0(\r)
	sw $a1,EOFFSET_A1(\r)
	sw $a2,EOFFSET_A2(\r)
	sw $a3,EOFFSET_A3(\r)
	sw $t0,EOFFSET_T0(\r)
	sw $t1,EOFFSET_T1(\r)
	sw $t2,EOFFSET_T2(\r)
	sw $t3,EOFFSET_T3(\r)
	sw $t4,EOFFSET_T4(\r)
	sw $t5,EOFFSET_T5(\r)
	sw $t6,EOFFSET_T6(\r)
	sw $t7,EOFFSET_T7(\r)
	sw $t8,EOFFSET_T8(\r)
	sw $t9,EOFFSET_T9(\r)

	mflo $at
	sw $at, EOFFSET_LO(\r)
	mfhi $at
	sw $at, EOFFSET_HI(\r)
	
#ifdef CONFIG_DEBUG_ALLREGS	
	sw $s0,EOFFSET_S0(\r)
	sw $s1,EOFFSET_S1(\r)
	sw $s2,EOFFSET_S2(\r)
	sw $s3,EOFFSET_S3(\r)
	sw $s4,EOFFSET_S4(\r)
	sw $s5,EOFFSET_S5(\r)
	sw $s6,EOFFSET_S6(\r)
	sw $s7,EOFFSET_S7(\r)
	sw $s8,EOFFSET_S8(\r)
#endif
	
	sw $gp,EOFFSET_GP(\r)
	sw $ra,EOFFSET_RA(\r)
	sw $k1,EOFFSET_K1(\r)

	mfc0 $t0, $status
	mfc0 $t1, $epc
	
	and $t2, $t0, REG_SAVE_MASK  # Save only KSU,EXL,ERL,IE
	li $t3, ~(0x1f)
	and $t0, $t0, $t3           # Clear KSU,EXL,ERL,IE
	
	sw $t2,EOFFSET_STATUS(\r)
	sw $t1,EOFFSET_EPC(\r)
	mtc0 $t0, $status
.endm

.macro REGISTERS_LOAD r
	# Update only UM,EXR,IE from status, the rest
	# is controlled by OS and not bound to task
	mfc0 $t0, $status
	lw $t1,EOFFSET_STATUS(\r)

	li $t2, ~REG_SAVE_MASK    # Mask UM,EXL,ERL,IE
	and $t0, $t0, $t2
	
	or $t0, $t0, $t1   # Copy UM,EXL,ERL,IE from saved status
	mtc0 $t0, $status
	
	lw $v0,EOFFSET_V0(\r)
	lw $v1,EOFFSET_V1(\r)
	lw $a0,EOFFSET_A0(\r)
	lw $a1,EOFFSET_A1(\r)
	lw $a2,EOFFSET_A2(\r)
	lw $a3,EOFFSET_A3(\r)
	lw $t0,EOFFSET_T0(\r)
	lw $t1,EOFFSET_T1(\r)
	lw $t2,EOFFSET_T2(\r)
	lw $t3,EOFFSET_T3(\r)
	lw $t4,EOFFSET_T4(\r)
	lw $t5,EOFFSET_T5(\r)
	lw $t6,EOFFSET_T6(\r)
	lw $t7,EOFFSET_T7(\r)
	lw $t8,EOFFSET_T8(\r)
	lw $t9,EOFFSET_T9(\r)
	
#ifdef CONFIG_DEBUG_ALLREGS	
	lw $s0,EOFFSET_S0(\r)
	lw $s1,EOFFSET_S1(\r)
	lw $s2,EOFFSET_S2(\r)
	lw $s3,EOFFSET_S3(\r)
	lw $s4,EOFFSET_S4(\r)
	lw $s5,EOFFSET_S5(\r)
	lw $s6,EOFFSET_S6(\r)
	lw $s7,EOFFSET_S7(\r)
	lw $s8,EOFFSET_S8(\r)
#endif
	lw $gp,EOFFSET_GP(\r)
	lw $ra,EOFFSET_RA(\r)
	lw $k1,EOFFSET_K1(\r)
	
	lw $at,EOFFSET_LO(\r)
	mtlo $at
	lw $at,EOFFSET_HI(\r)
	mthi $at

	lw $at,EOFFSET_EPC(\r)
	mtc0 $at, $epc
	
	lw $at,EOFFSET_AT(\r)
	lw $sp,EOFFSET_SP(\r)
.endm

# Move kernel stack pointer address to register K0
# - if we are in user mode, load the appropriate stack
# address
.macro KERNEL_STACK_TO_K0
	# If we are in user mode
	mfc0 $k0, $status
	andi $k0, 0x10
	
	beq $k0, $0, 1f
	add $k0, $sp, 0
	
	# Move $k0 pointer to kernel stack
	lui $k0, %hi(supervisor_sp)
	ori $k0, $k0, %lo(supervisor_sp)
	# Move $k0 (superveisor_sp)
	lw $k0, 0($k0)
1:		
.endm
		
.org 0x0
kernel_image_start:
	/* Load temporary stack */
	lui $sp, %hi(end_stack)
	ori $sp, $sp, %lo(end_stack)
	
	/* $a1 contains physical address of bootinfo_t */
	/* $a2 contains size of bootinfo_t */
	
	beq $a2, $0, bootinfo_end
	
	/* Not sure about this, but might be needed for PIC code???? */
	lui $gp, 0x8000
	
	lui $a3, %hi(bootinfo)
	ori $a3, $a3, %lo(bootinfo)
	
	bootinfo_loop:
		
		lw $v0, 0($a1)
		sw $v0, 0($a3)
		
		addi $a1, $a1, 4
		addi $a3, $a3, 4
		addi $a2, $a2, -4
		
		bgtz $a2, bootinfo_loop
		nop
		
	bootinfo_end:
	
	jal arch_pre_main
	nop
	
	j main_bsp
	nop

	.space TEMP_STACK_SIZE
end_stack:

tlb_refill_entry:
	j tlb_refill_handler
	nop

cache_error_entry:
	j cache_error_handler
	nop

exception_entry:
	j exception_handler
	nop	

	
	
exception_handler:
	KERNEL_STACK_TO_K0
	sub $k0, REGISTER_SPACE
	sw $sp,EOFFSET_SP($k0)
	move $sp, $k0
	
	mfc0 $k0, $cause
	
	sra $k0, $k0, 0x2     # cp0_exc_cause() part 1
	andi $k0, $k0, 0x1f   # cp0_exc_cause() part 2
	sub $k0, 8            # 8=SYSCALL
	
	beqz $k0, syscall_shortcut
	add $k0, 8            # Revert $k0 back to correct exc number
	
	REGISTERS_STORE_AND_EXC_RESET $sp
	
	move $a1, $sp
	jal exc_dispatch      # exc_dispatch(excno, register_space)
	move $a0, $k0

	REGISTERS_LOAD $sp
	# The $sp is automatically restored to former value
	eret

# it seems that mips reserves some space on stack for varfuncs???
#define SS_ARG4   16
#define SS_SP     EOFFSET_SP
#define SS_STATUS EOFFSET_STATUS
#define SS_EPC    EOFFSET_EPC
#define SS_K1     EOFFSET_K1
syscall_shortcut:
	# We have a lot of space on the stack, with free use
	mfc0 $t1, $epc
	mfc0 $t0, $status
	sw $t1,SS_EPC($sp)  # Save EPC
	sw $k1,SS_K1($sp)   # Save k1, which is not saved during context switch
	
	and $t2, $t0, REG_SAVE_MASK # Save only KSU,EXL,ERL,IE
	li $t3, ~(0x1f)
	and $t0, $t0, $t3           # Clear KSU,EXL,ERL
	ori $t0, $t0, 0x1           # Set IE

	sw $t2,SS_STATUS($sp)
	mtc0 $t0, $status

	# CALL Syscall handler
	jal syscall_handler
	sw $v0, SS_ARG4($sp)        # save v0 - arg4 to stack

	# restore status
	mfc0 $t0, $status
	lw $t1,SS_STATUS($sp)

	# Change back to EXL=1(from last exception), otherwise
	# an interrupt could rewrite the CP0-EPC
	li $t2, ~REG_SAVE_MASK      # Mask UM,EXL,ERL,IE
	and $t0, $t0, $t2
	or $t0, $t0, $t1            # Copy UM,EXL,ERL,IE from saved status
	mtc0 $t0, $status
			
	# restore epc+4
	lw $t0,SS_EPC($sp)
	lw $k1,SS_K1($sp)
	addi $t0, $t0, 4
	mtc0 $t0, $epc
	
	lw $sp,SS_SP($sp) # restore sp
	
	eret
		
tlb_refill_handler:
	KERNEL_STACK_TO_K0
	sub $k0, REGISTER_SPACE
	REGISTERS_STORE_AND_EXC_RESET $k0
	sw $sp,EOFFSET_SP($k0)
	add $sp, $k0, 0

	jal tlb_refill /* tlb_refill(register_space) */
	add $a0, $sp, 0 

	REGISTERS_LOAD $sp

	eret

cache_error_handler:
	KERNEL_STACK_TO_K0
	sub $k0, REGISTER_SPACE
	REGISTERS_STORE_AND_EXC_RESET $k0
	sw $sp,EOFFSET_SP($k0)
	add $sp, $k0, 0

	jal cache_error
	nop

	REGISTERS_LOAD $sp

	eret

userspace_asm:
	add $sp, $a0, 0
	add $v0, $a1, 0 
	add $t9, $a2, 0   # Set up correct entry into PIC code 
	eret
