#
# Copyright (C) 2005 Jakub Jermar
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

/**
 * This file contains kernel trap table.
 */

.register %g2, #scratch
.register %g3, #scratch

.text

#include <arch/trap/trap_table.h>
#include <arch/trap/regwin.h>
#include <arch/trap/interrupt.h>
#include <arch/trap/exception.h>
#include <arch/trap/syscall.h>
#include <arch/trap/mmu.h>
#include <arch/mm/mmu.h>
#include <arch/mm/page.h>
#include <arch/stack.h>
#include <arch/regdef.h>

#define TABLE_SIZE	TRAP_TABLE_SIZE
#define ENTRY_SIZE	TRAP_TABLE_ENTRY_SIZE

/*
 * Kernel trap table.
 */
.align TABLE_SIZE
.global trap_table
trap_table:

/* TT = 0x08, TL = 0, instruction_access_exception */
.org trap_table + TT_INSTRUCTION_ACCESS_EXCEPTION*ENTRY_SIZE
.global instruction_access_exception
instruction_access_exception:
	PREEMPTIBLE_HANDLER do_instruction_access_exc

/* TT = 0x10, TL = 0, illegal_instruction */
.org trap_table + TT_ILLEGAL_INSTRUCTION*ENTRY_SIZE
.global illegal_instruction
illegal_instruction:
	PREEMPTIBLE_HANDLER do_illegal_instruction

/* TT = 0x24, TL = 0, clean_window handler */
.org trap_table + TT_CLEAN_WINDOW*ENTRY_SIZE
.global clean_window_handler
clean_window_handler:
	CLEAN_WINDOW_HANDLER

/* TT = 0x32, TL = 0, data_access_error */
.org trap_table + TT_DATA_ACCESS_ERROR*ENTRY_SIZE
.global data_access_error
data_access_error:
	PREEMPTIBLE_HANDLER do_data_access_error

/* TT = 0x34, TL = 0, mem_address_not_aligned */
.org trap_table + TT_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global mem_address_not_aligned
mem_address_not_aligned:
	PREEMPTIBLE_HANDLER do_mem_address_not_aligned

/* TT = 0x41, TL = 0, interrupt_level_1 handler */
.org trap_table + TT_INTERRUPT_LEVEL_1*ENTRY_SIZE
.global interrupt_level_1_handler
interrupt_level_1_handler:
	INTERRUPT_LEVEL_N_HANDLER 1

/* TT = 0x42, TL = 0, interrupt_level_2 handler */
.org trap_table + TT_INTERRUPT_LEVEL_2*ENTRY_SIZE
.global interrupt_level_2_handler
interrupt_level_2_handler:
	INTERRUPT_LEVEL_N_HANDLER 2

/* TT = 0x43, TL = 0, interrupt_level_3 handler */
.org trap_table + TT_INTERRUPT_LEVEL_3*ENTRY_SIZE
.global interrupt_level_3_handler
interrupt_level_3_handler:
	INTERRUPT_LEVEL_N_HANDLER 3

/* TT = 0x44, TL = 0, interrupt_level_4 handler */
.org trap_table + TT_INTERRUPT_LEVEL_4*ENTRY_SIZE
.global interrupt_level_4_handler
interrupt_level_4_handler:
	INTERRUPT_LEVEL_N_HANDLER 4

/* TT = 0x45, TL = 0, interrupt_level_5 handler */
.org trap_table + TT_INTERRUPT_LEVEL_5*ENTRY_SIZE
.global interrupt_level_5_handler
interrupt_level_5_handler:
	INTERRUPT_LEVEL_N_HANDLER 5

/* TT = 0x46, TL = 0, interrupt_level_6 handler */
.org trap_table + TT_INTERRUPT_LEVEL_6*ENTRY_SIZE
.global interrupt_level_6_handler
interrupt_level_6_handler:
	INTERRUPT_LEVEL_N_HANDLER 6

/* TT = 0x47, TL = 0, interrupt_level_7 handler */
.org trap_table + TT_INTERRUPT_LEVEL_7*ENTRY_SIZE
.global interrupt_level_7_handler
interrupt_level_7_handler:
	INTERRUPT_LEVEL_N_HANDLER 7

/* TT = 0x48, TL = 0, interrupt_level_8 handler */
.org trap_table + TT_INTERRUPT_LEVEL_8*ENTRY_SIZE
.global interrupt_level_8_handler
interrupt_level_8_handler:
	INTERRUPT_LEVEL_N_HANDLER 8

/* TT = 0x49, TL = 0, interrupt_level_9 handler */
.org trap_table + TT_INTERRUPT_LEVEL_9*ENTRY_SIZE
.global interrupt_level_9_handler
interrupt_level_9_handler:
	INTERRUPT_LEVEL_N_HANDLER 9

/* TT = 0x4a, TL = 0, interrupt_level_10 handler */
.org trap_table + TT_INTERRUPT_LEVEL_10*ENTRY_SIZE
.global interrupt_level_10_handler
interrupt_level_10_handler:
	INTERRUPT_LEVEL_N_HANDLER 10

/* TT = 0x4b, TL = 0, interrupt_level_11 handler */
.org trap_table + TT_INTERRUPT_LEVEL_11*ENTRY_SIZE
.global interrupt_level_11_handler
interrupt_level_11_handler:
	INTERRUPT_LEVEL_N_HANDLER 11

/* TT = 0x4c, TL = 0, interrupt_level_12 handler */
.org trap_table + TT_INTERRUPT_LEVEL_12*ENTRY_SIZE
.global interrupt_level_12_handler
interrupt_level_12_handler:
	INTERRUPT_LEVEL_N_HANDLER 12

/* TT = 0x4d, TL = 0, interrupt_level_13 handler */
.org trap_table + TT_INTERRUPT_LEVEL_13*ENTRY_SIZE
.global interrupt_level_13_handler
interrupt_level_13_handler:
	INTERRUPT_LEVEL_N_HANDLER 13

/* TT = 0x4e, TL = 0, interrupt_level_14 handler */
.org trap_table + TT_INTERRUPT_LEVEL_14*ENTRY_SIZE
.global interrupt_level_14_handler
interrupt_level_14_handler:
	INTERRUPT_LEVEL_N_HANDLER 14

/* TT = 0x4f, TL = 0, interrupt_level_15 handler */
.org trap_table + TT_INTERRUPT_LEVEL_15*ENTRY_SIZE
.global interrupt_level_15_handler
interrupt_level_15_handler:
	INTERRUPT_LEVEL_N_HANDLER 15

/* TT = 0x60, TL = 0, interrupt_vector_trap handler */
.org trap_table + TT_INTERRUPT_VECTOR_TRAP*ENTRY_SIZE
.global interrupt_vector_trap_handler
interrupt_vector_trap_handler:
	INTERRUPT_VECTOR_TRAP_HANDLER

/* TT = 0x64, TL = 0, fast_instruction_access_MMU_miss */
.org trap_table + TT_FAST_INSTRUCTION_ACCESS_MMU_MISS*ENTRY_SIZE
.global fast_instruction_access_mmu_miss_handler
fast_instruction_access_mmu_miss_handler:
	FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER

/* TT = 0x68, TL = 0, fast_data_access_MMU_miss */
.org trap_table + TT_FAST_DATA_ACCESS_MMU_MISS*ENTRY_SIZE
.global fast_data_access_mmu_miss_handler
fast_data_access_mmu_miss_handler:
	FAST_DATA_ACCESS_MMU_MISS_HANDLER

/* TT = 0x6c, TL = 0, fast_data_access_protection */
.org trap_table + TT_FAST_DATA_ACCESS_PROTECTION*ENTRY_SIZE
.global fast_data_access_protection_handler
fast_data_access_protection_handler:
	FAST_DATA_ACCESS_PROTECTION_HANDLER

/* TT = 0x80, TL = 0, spill_0_normal handler */
.org trap_table + TT_SPILL_0_NORMAL*ENTRY_SIZE
.global spill_0_normal
spill_0_normal:
	SPILL_NORMAL_HANDLER_KERNEL

/* TT = 0x84, TL = 0, spill_1_normal handler */
.org trap_table + TT_SPILL_1_NORMAL*ENTRY_SIZE
.global spill_1_normal
spill_1_normal:
	SPILL_NORMAL_HANDLER_USERSPACE

/* TT = 0x88, TL = 0, spill_2_normal handler */
.org trap_table + TT_SPILL_2_NORMAL*ENTRY_SIZE
.global spill_2_normal
spill_2_normal:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xa0, TL = 0, spill_0_other handler */
.org trap_table + TT_SPILL_0_OTHER*ENTRY_SIZE
.global spill_0_other
spill_0_other:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xc0, TL = 0, fill_0_normal handler */
.org trap_table + TT_FILL_0_NORMAL*ENTRY_SIZE
.global fill_0_normal
fill_0_normal:
	FILL_NORMAL_HANDLER_KERNEL

/* TT = 0xc4, TL = 0, fill_1_normal handler */
.org trap_table + TT_FILL_1_NORMAL*ENTRY_SIZE
.global fill_1_normal
fill_1_normal:
	FILL_NORMAL_HANDLER_USERSPACE

/* TT = 0x100, TL = 0, trap_instruction_0 */
.org trap_table + TT_TRAP_INSTRUCTION(0)*ENTRY_SIZE
.global trap_instruction_0
trap_instruction_0:
	TRAP_INSTRUCTION 0

/* TT = 0x101, TL = 0, trap_instruction_1 */
.org trap_table + TT_TRAP_INSTRUCTION(1)*ENTRY_SIZE
.global trap_instruction_1
trap_instruction_1:
	TRAP_INSTRUCTION 1

/* TT = 0x102, TL = 0, trap_instruction_2 */
.org trap_table + TT_TRAP_INSTRUCTION(2)*ENTRY_SIZE
.global trap_instruction_2
trap_instruction_2:
	TRAP_INSTRUCTION 2

/* TT = 0x103, TL = 0, trap_instruction_3 */
.org trap_table + TT_TRAP_INSTRUCTION(3)*ENTRY_SIZE
.global trap_instruction_3
trap_instruction_3:
	TRAP_INSTRUCTION 3

/* TT = 0x104, TL = 0, trap_instruction_4 */
.org trap_table + TT_TRAP_INSTRUCTION(4)*ENTRY_SIZE
.global trap_instruction_4
trap_instruction_4:
	TRAP_INSTRUCTION 4

/* TT = 0x105, TL = 0, trap_instruction_5 */
.org trap_table + TT_TRAP_INSTRUCTION(5)*ENTRY_SIZE
.global trap_instruction_5
trap_instruction_5:
	TRAP_INSTRUCTION 5

/* TT = 0x106, TL = 0, trap_instruction_6 */
.org trap_table + TT_TRAP_INSTRUCTION(6)*ENTRY_SIZE
.global trap_instruction_6
trap_instruction_6:
	TRAP_INSTRUCTION 6

/* TT = 0x107, TL = 0, trap_instruction_7 */
.org trap_table + TT_TRAP_INSTRUCTION(7)*ENTRY_SIZE
.global trap_instruction_7
trap_instruction_7:
	TRAP_INSTRUCTION 7

/* TT = 0x108, TL = 0, trap_instruction_8 */
.org trap_table + TT_TRAP_INSTRUCTION(8)*ENTRY_SIZE
.global trap_instruction_8
trap_instruction_8:
	TRAP_INSTRUCTION 8

/* TT = 0x109, TL = 0, trap_instruction_9 */
.org trap_table + TT_TRAP_INSTRUCTION(9)*ENTRY_SIZE
.global trap_instruction_9
trap_instruction_9:
	TRAP_INSTRUCTION 9

/* TT = 0x10a, TL = 0, trap_instruction_10 */
.org trap_table + TT_TRAP_INSTRUCTION(10)*ENTRY_SIZE
.global trap_instruction_10
trap_instruction_10:
	TRAP_INSTRUCTION 10

/* TT = 0x10b, TL = 0, trap_instruction_11 */
.org trap_table + TT_TRAP_INSTRUCTION(11)*ENTRY_SIZE
.global trap_instruction_11
trap_instruction_11:
	TRAP_INSTRUCTION 11

/* TT = 0x10c, TL = 0, trap_instruction_12 */
.org trap_table + TT_TRAP_INSTRUCTION(12)*ENTRY_SIZE
.global trap_instruction_12
trap_instruction_12:
	TRAP_INSTRUCTION 12

/* TT = 0x10d, TL = 0, trap_instruction_13 */
.org trap_table + TT_TRAP_INSTRUCTION(13)*ENTRY_SIZE
.global trap_instruction_13
trap_instruction_13:
	TRAP_INSTRUCTION 13

/* TT = 0x10e, TL = 0, trap_instruction_14 */
.org trap_table + TT_TRAP_INSTRUCTION(14)*ENTRY_SIZE
.global trap_instruction_14
trap_instruction_14:
	TRAP_INSTRUCTION 14

/* TT = 0x10f, TL = 0, trap_instruction_15 */
.org trap_table + TT_TRAP_INSTRUCTION(15)*ENTRY_SIZE
.global trap_instruction_15
trap_instruction_15:
	TRAP_INSTRUCTION 15

/* TT = 0x110, TL = 0, trap_instruction_16 */
.org trap_table + TT_TRAP_INSTRUCTION(16)*ENTRY_SIZE
.global trap_instruction_16
trap_instruction_16:
	TRAP_INSTRUCTION 16

/* TT = 0x111, TL = 0, trap_instruction_17 */
.org trap_table + TT_TRAP_INSTRUCTION(17)*ENTRY_SIZE
.global trap_instruction_17
trap_instruction_17:
	TRAP_INSTRUCTION 17

/* TT = 0x112, TL = 0, trap_instruction_18 */
.org trap_table + TT_TRAP_INSTRUCTION(18)*ENTRY_SIZE
.global trap_instruction_18
trap_instruction_18:
	TRAP_INSTRUCTION 18

/* TT = 0x113, TL = 0, trap_instruction_19 */
.org trap_table + TT_TRAP_INSTRUCTION(19)*ENTRY_SIZE
.global trap_instruction_19
trap_instruction_19:
	TRAP_INSTRUCTION 19

/* TT = 0x114, TL = 0, trap_instruction_20 */
.org trap_table + TT_TRAP_INSTRUCTION(20)*ENTRY_SIZE
.global trap_instruction_20
trap_instruction_20:
	TRAP_INSTRUCTION 20

/* TT = 0x115, TL = 0, trap_instruction_21 */
.org trap_table + TT_TRAP_INSTRUCTION(21)*ENTRY_SIZE
.global trap_instruction_21
trap_instruction_21:
	TRAP_INSTRUCTION 21

/* TT = 0x116, TL = 0, trap_instruction_22 */
.org trap_table + TT_TRAP_INSTRUCTION(22)*ENTRY_SIZE
.global trap_instruction_22
trap_instruction_22:
	TRAP_INSTRUCTION 22

/* TT = 0x117, TL = 0, trap_instruction_23 */
.org trap_table + TT_TRAP_INSTRUCTION(23)*ENTRY_SIZE
.global trap_instruction_23
trap_instruction_23:
	TRAP_INSTRUCTION 23

/* TT = 0x118, TL = 0, trap_instruction_24 */
.org trap_table + TT_TRAP_INSTRUCTION(24)*ENTRY_SIZE
.global trap_instruction_24
trap_instruction_24:
	TRAP_INSTRUCTION 24

/* TT = 0x119, TL = 0, trap_instruction_25 */
.org trap_table + TT_TRAP_INSTRUCTION(25)*ENTRY_SIZE
.global trap_instruction_25
trap_instruction_25:
	TRAP_INSTRUCTION 25

/* TT = 0x11a, TL = 0, trap_instruction_26 */
.org trap_table + TT_TRAP_INSTRUCTION(26)*ENTRY_SIZE
.global trap_instruction_26
trap_instruction_26:
	TRAP_INSTRUCTION 26

/* TT = 0x11b, TL = 0, trap_instruction_27 */
.org trap_table + TT_TRAP_INSTRUCTION(27)*ENTRY_SIZE
.global trap_instruction_27
trap_instruction_27:
	TRAP_INSTRUCTION 27

/* TT = 0x11c, TL = 0, trap_instruction_28 */
.org trap_table + TT_TRAP_INSTRUCTION(28)*ENTRY_SIZE
.global trap_instruction_28
trap_instruction_28:
	TRAP_INSTRUCTION 28

/* TT = 0x11d, TL = 0, trap_instruction_29 */
.org trap_table + TT_TRAP_INSTRUCTION(29)*ENTRY_SIZE
.global trap_instruction_29
trap_instruction_29:
	TRAP_INSTRUCTION 29

/* TT = 0x11e, TL = 0, trap_instruction_30 */
.org trap_table + TT_TRAP_INSTRUCTION(30)*ENTRY_SIZE
.global trap_instruction_30
trap_instruction_30:
	TRAP_INSTRUCTION 30

/* TT = 0x11f, TL = 0, trap_instruction_31 */
.org trap_table + TT_TRAP_INSTRUCTION(31)*ENTRY_SIZE
.global trap_instruction_31
trap_instruction_31:
	TRAP_INSTRUCTION 31

/*
 * Handlers for TL>0.
 */

/* TT = 0x08, TL > 0, instruction_access_exception */
.org trap_table + (TT_INSTRUCTION_ACCESS_EXCEPTION+512)*ENTRY_SIZE
.global instruction_access_exception_high
instruction_access_exception_high:
	PREEMPTIBLE_HANDLER do_instruction_access_exc

/* TT = 0x10, TL > 0, illegal_instruction */
.org trap_table + (TT_ILLEGAL_INSTRUCTION+512)*ENTRY_SIZE
.global illegal_instruction_high
illegal_instruction_high:
	PREEMPTIBLE_HANDLER do_illegal_instruction

/* TT = 0x24, TL > 0, clean_window handler */
.org trap_table + (TT_CLEAN_WINDOW+512)*ENTRY_SIZE
.global clean_window_handler_high
clean_window_handler_high:
	CLEAN_WINDOW_HANDLER

/* TT = 0x32, TL > 0, data_access_error */
.org trap_table + (TT_DATA_ACCESS_ERROR+512)*ENTRY_SIZE
.global data_access_error_high
data_access_error_high:
	PREEMPTIBLE_HANDLER do_data_access_error

/* TT = 0x34, TL > 0, mem_address_not_aligned */
.org trap_table + (TT_MEM_ADDRESS_NOT_ALIGNED+512)*ENTRY_SIZE
.global mem_address_not_aligned_high
mem_address_not_aligned_high:
	PREEMPTIBLE_HANDLER do_mem_address_not_aligned

/* TT = 0x64, TL > 0, fast_instruction_access_MMU_miss */
.org trap_table + (TT_FAST_INSTRUCTION_ACCESS_MMU_MISS+512)*ENTRY_SIZE
.global fast_instruction_access_mmu_miss_handler_high
fast_instruction_access_mmu_miss_handler_high:
	FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER

/* TT = 0x68, TL > 0, fast_data_access_MMU_miss */
.org trap_table + (TT_FAST_DATA_ACCESS_MMU_MISS+512)*ENTRY_SIZE
.global fast_data_access_mmu_miss_handler_high
fast_data_access_mmu_miss_handler_high:
	FAST_DATA_ACCESS_MMU_MISS_HANDLER

/* TT = 0x6c, TL > 0, fast_data_access_protection */
.org trap_table + (TT_FAST_DATA_ACCESS_PROTECTION+512)*ENTRY_SIZE
.global fast_data_access_protection_handler_high
fast_data_access_protection_handler_high:
	FAST_DATA_ACCESS_PROTECTION_HANDLER

/* TT = 0x80, TL > 0, spill_0_normal handler */
.org trap_table + (TT_SPILL_0_NORMAL+512)*ENTRY_SIZE
.global spill_0_normal_high
spill_0_normal_high:
	SPILL_NORMAL_HANDLER_KERNEL

/* TT = 0x88, TL > 0, spill_2_normal handler */
.org trap_table + (TT_SPILL_2_NORMAL+512)*ENTRY_SIZE
.global spill_2_normal_high
spill_2_normal_high:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xa0, TL > 0, spill_0_other handler */
.org trap_table + (TT_SPILL_0_OTHER+512)*ENTRY_SIZE
.global spill_0_other_high
spill_0_other_high:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xc0, TL > 0, fill_0_normal handler */
.org trap_table + (TT_FILL_0_NORMAL+512)*ENTRY_SIZE
.global fill_0_normal_high
fill_0_normal_high:
	FILL_NORMAL_HANDLER_KERNEL

#define NOT(x)	((x) == 0)

/* Preemptible trap handler for TL=1.
 *
 * This trap handler makes arrangements to make calling of scheduler() from
 * within a trap context possible. It is called from several other trap
 * handlers.
 *
 * This function can be entered either with interrupt globals or alternate globals.
 * Memory management trap handlers are obliged to switch to one of those global sets
 * prior to calling this function. Register window management functions are not
 * allowed to modify the alternate global registers.
 *
 * Input registers:
 *	%g1		Address of function to call.
 * 	%g2	 	First argument for the function.
 *	%g6		Pre-set as kernel stack base if trap from userspace.
 *	%g7		Pre-set as address of the userspace window buffer.
 */
.macro PREEMPTIBLE_HANDLER_TEMPLATE is_syscall
.if NOT(\is_syscall)
	rdpr %tstate, %g3
	andcc %g3, TSTATE_PRIV_BIT, %g0		! if this trap came from the privileged mode...
	bnz 0f					! ...skip setting of kernel stack and primary context
	nop
.endif
	/*
	 * Normal window spills will go to the userspace window buffer.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(2), %wstate

	/*
	 * Switch to kernel stack. The old stack is
	 * automatically saved in the old window's %sp
	 * and the new window's %fp.
	 */
	save %g6, -PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE, %sp

.if \is_syscall
	/*
	 * Copy arguments for the syscall to the new window.
	 */
	mov %i0, %o2
	mov %i1, %o3
	mov %i2, %o4
	mov %i3, %o5
.endif

	/*
	 * Mark the CANRESTORE windows as OTHER windows.
	 * Set CLEANWIN to NWINDOW-1 so that clean_window traps do not occur.
	 */
	rdpr %canrestore, %l0
	wrpr %l0, %otherwin
	wrpr %g0, %canrestore
	wrpr %g0, NWINDOW - 1, %cleanwin

	/*
	 * Switch to primary context 0.
	 */
	mov VA_PRIMARY_CONTEXT_REG, %l0
	stxa %g0, [%l0] ASI_DMMU
	rd %pc, %l0
	flush %l0

.if NOT(\is_syscall)
	ba 1f
	nop

0:
	save %sp, -PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE, %sp

	/*
	 * At this moment, we are using the kernel stack 
	 * and have successfully allocated a register window.
	 */
1:
.endif
	/*
	 * Other window spills will go to the userspace window buffer
	 * and normal spills will go to the kernel stack.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(0), %wstate
	
	/*
	 * Copy arguments.
	 */
	mov %g1, %l0
	mov %g2, %o0

	/*
	 * Save TSTATE, TPC and TNPC aside.
	 */
	rdpr %tstate, %g1
	rdpr %tpc, %g2
	rdpr %tnpc, %g3

	/*
	 * The following memory accesses will not fault
	 * because special provisions are made to have
	 * the kernel stack of THREAD locked in DTLB.
	 */
	stx %g1, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TSTATE]
	stx %g2, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TPC]
	stx %g3, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TNPC]
	
	wrpr %g0, 0, %tl
	wrpr %g0, PSTATE_PRIV_BIT, %pstate
	SAVE_GLOBALS
	
	/*
	 * Call the higher-level handler and pass istate as second parameter.
	 */
	call %l0
	add %sp, PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TNPC, %o1

.if \is_syscall
	/*
	 * Copy the value returned by the syscall.
	 */
	mov %o0, %i0
.endif

	RESTORE_GLOBALS
	wrpr %g0, PSTATE_AG_BIT | PSTATE_PRIV_BIT, %pstate
	wrpr %g0, 1, %tl
	
	/*
	 * Read TSTATE, TPC and TNPC from saved copy.
	 */
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TSTATE], %g1
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TPC], %g2
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TNPC], %g3

	/*
	 * Restore TSTATE, TPC and TNPC from saved copies.
	 */
	wrpr %g1, 0, %tstate
	wrpr %g2, 0, %tpc
	wrpr %g3, 0, %tnpc

	/*
	 * If OTHERWIN is zero, then all the userspace windows have been
	 * spilled to kernel memory (i.e. register window buffer). Moreover,
	 * if the scheduler was called in the meantime, all valid windows
	 * belonging to other threads were spilled by context_restore().
	 * If OTHERWIN is non-zero, then some userspace windows are still
	 * valid. Others might have been spilled. However, the CWP pointer
	 * needs no fixing because the scheduler had not been called.
	 */
	rdpr %otherwin, %l0
	brnz %l0, 0f
	nop

	/*
	 * OTHERWIN == 0
	 */

	/*
	 * If TSTATE.CWP + 1 == CWP, then we still do not have to fix CWP.
	 */
	and %g1, TSTATE_CWP_MASK, %l0
	inc %l0
	and %l0, NWINDOW - 1, %l0	! %l0 mod NWINDOW
	rdpr %cwp, %l1
	cmp %l0, %l1
	bz 0f				! CWP is ok
	nop

	/*
	 * Fix CWP.
	 * In order to recapitulate, the input registers in the current
	 * window are the output registers of the window to which we want
	 * to restore. Because the fill trap fills only input and local
	 * registers of a window, we need to preserve those output
	 * registers manually.
	 */
	mov %sp, %g2
	stx %i0, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I0]
	stx %i1, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I1]
	stx %i2, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I2]
	stx %i3, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I3]
	stx %i4, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I4]
	stx %i5, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I5]
	stx %i6, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I6]
	stx %i7, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I7]
	wrpr %l0, 0, %cwp
	mov %g2, %sp
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I0], %i0
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I1], %i1
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I2], %i2
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I3], %i3
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I4], %i4
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I5], %i5
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I6], %i6
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_I7], %i7

	/*
	 * OTHERWIN != 0 or fall-through from the OTHERWIN == 0 case.
	 * The CWP has already been restored to the value it had prior to the SAVE
	 * at the beginning of this function.
	 */
0:
.if NOT(\is_syscall)
	rdpr %tstate, %g1
	andcc %g1, TSTATE_PRIV_BIT, %g0		! if we are not returning to userspace...,
	bnz 1f					! ...skip restoring userspace windows
	nop
.endif

	/*
	 * Spills and fills will be processed by the {spill,fill}_1_normal
	 * handlers.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(1), %wstate

	/*
	 * Set primary context according to secondary context.
	 */
	wr %g0, ASI_DMMU, %asi
	ldxa [VA_SECONDARY_CONTEXT_REG] %asi, %g1
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi
	flush %o7
	
	rdpr %cwp, %g1
	rdpr %otherwin, %g2

	/*
	 * Skip all OTHERWIN windows and descend to the first window
	 * in the userspace window buffer.
	 */
	sub %g1, %g2, %g3
	dec %g3
	and %g3, NWINDOW - 1, %g3
	wrpr %g3, 0, %cwp

	/*
	 * CWP is now in the window last saved in the userspace window buffer.
	 * Fill all windows stored in the buffer.
	 */
	clr %g4
	set PAGE_SIZE - 1, %g5
0:	andcc %g7, %g5, %g0			! PAGE_SIZE alignment check
	bz 0f					! %g7 is page-aligned, no more windows to refill
	nop

	add %g7, -STACK_WINDOW_SAVE_AREA_SIZE, %g7
	ldx [%g7 + L0_OFFSET], %l0
	ldx [%g7 + L1_OFFSET], %l1
	ldx [%g7 + L2_OFFSET], %l2
	ldx [%g7 + L3_OFFSET], %l3
	ldx [%g7 + L4_OFFSET], %l4
	ldx [%g7 + L5_OFFSET], %l5
	ldx [%g7 + L6_OFFSET], %l6
	ldx [%g7 + L7_OFFSET], %l7
	ldx [%g7 + I0_OFFSET], %i0
	ldx [%g7 + I1_OFFSET], %i1
	ldx [%g7 + I2_OFFSET], %i2
	ldx [%g7 + I3_OFFSET], %i3
	ldx [%g7 + I4_OFFSET], %i4
	ldx [%g7 + I5_OFFSET], %i5
	ldx [%g7 + I6_OFFSET], %i6
	ldx [%g7 + I7_OFFSET], %i7

	dec %g3
	and %g3, NWINDOW - 1, %g3
	wrpr %g3, 0, %cwp			! switch to the preceeding window

	ba 0b
	inc %g4

0:
	/*
	 * Switch back to the proper current window and adjust
	 * OTHERWIN, CANRESTORE, CANSAVE and CLEANWIN.
	 */
	wrpr %g1, 0, %cwp
	add %g4, %g2, %g2
	cmp %g2, NWINDOW - 2
	bg 2f					! fix the CANRESTORE=NWINDOW-1 anomaly
	mov NWINDOW - 2, %g1			! use dealy slot for both cases
	sub %g1, %g2, %g1
	
	wrpr %g0, 0, %otherwin
	wrpr %g1, 0, %cansave			! NWINDOW - 2 - CANRESTORE
	wrpr %g2, 0, %canrestore		! OTHERWIN + windows in the buffer
	wrpr %g2, 0, %cleanwin			! avoid information leak

1:
	restore

.if \is_syscall
	done
.else
	retry
.endif

	/*
	 * We got here in order to avoid inconsistency of the window state registers.
	 * If the:
	 *
	 * 	save %g6, -PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE, %sp
	 *
	 * instruction trapped and spilled a register window into the userspace
	 * window buffer, we have just restored NWINDOW - 1 register windows.
	 * However, CANRESTORE can be only NWINDOW - 2 at most.
	 *
	 * The solution is to manually switch to (CWP - 1) mod NWINDOW
	 * and set the window state registers so that:
	 *
	 * 	CANRESTORE 	= NWINDOW - 2
	 *	CLEANWIN	= NWINDOW - 2
	 *	CANSAVE 	= 0
	 *	OTHERWIN	= 0
	 *
	 * The RESTORE isntruction is therfore to be skipped.
	 */
2:
	wrpr %g0, 0, %otherwin
	wrpr %g0, 0, %cansave
	wrpr %g1, 0, %canrestore
	wrpr %g1, 0, %cleanwin

	rdpr %cwp, %g1
	dec %g1
	and %g1, NWINDOW - 1, %g1
	wrpr %g1, 0, %cwp			! CWP--
	
.if \is_syscall
	done
.else
	retry
.endif

.endm

.global preemptible_handler
preemptible_handler:
	PREEMPTIBLE_HANDLER_TEMPLATE 0

.global trap_instruction_handler
trap_instruction_handler:
	PREEMPTIBLE_HANDLER_TEMPLATE 1
