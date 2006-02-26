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
.register %g6, #scratch
.register %g7, #scratch 

.text

#include <arch/trap/trap_table.h>
#include <arch/trap/regwin.h>
#include <arch/trap/interrupt.h>
#include <arch/trap/exception.h>
#include <arch/trap/mmu.h>
#include <arch/stack.h>

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
	SIMPLE_HANDLER do_instruction_access_exc

/* TT = 0x10, TL = 0, illegal_instruction */
.org trap_table + TT_ILLEGAL_INSTRUCTION*ENTRY_SIZE
.global illegal_instruction
illegal_instruction:
	SIMPLE_HANDLER do_illegal_instruction

/* TT = 0x24, TL = 0, clean_window handler */
.org trap_table + TT_CLEAN_WINDOW*ENTRY_SIZE
.global clean_window_handler
clean_window_handler:
	CLEAN_WINDOW_HANDLER

/* TT = 0x34, TL = 0, mem_address_not_aligned */
.org trap_table + TT_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global mem_address_not_aligned
mem_address_not_aligned:
	SIMPLE_HANDLER do_mem_address_not_aligned

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
	SPILL_NORMAL_HANDLER

/* TT = 0xc0, TL = 0, fill_0_normal handler */
.org trap_table + TT_FILL_0_NORMAL*ENTRY_SIZE
.global fill_0_normal
fill_0_normal:
	FILL_NORMAL_HANDLER

/*
 * Handlers for TL>0.
 */

/* TT = 0x08, TL > 0, instruction_access_exception */
.org trap_table + (TT_INSTRUCTION_ACCESS_EXCEPTION+512)*ENTRY_SIZE
.global instruction_access_exception_high
instruction_access_exception_high:
	SIMPLE_HANDLER do_instruction_access_exc

/* TT = 0x10, TL > 0, illegal_instruction */
.org trap_table + (TT_ILLEGAL_INSTRUCTION+512)*ENTRY_SIZE
.global illegal_instruction_high
illegal_instruction_high:
	SIMPLE_HANDLER do_illegal_instruction

/* TT = 0x24, TL > 0, clean_window handler */
.org trap_table + (TT_CLEAN_WINDOW+512)*ENTRY_SIZE
.global clean_window_handler_high
clean_window_handler_high:
	CLEAN_WINDOW_HANDLER

/* TT = 0x34, TL > 0, mem_address_not_aligned */
.org trap_table + (TT_MEM_ADDRESS_NOT_ALIGNED+512)*ENTRY_SIZE
.global mem_address_not_aligned_high
mem_address_not_aligned_high:
	SIMPLE_HANDLER do_mem_address_not_aligned

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
	SPILL_NORMAL_HANDLER

/* TT = 0xc0, TL > 0, fill_0_normal handler */
.org trap_table + (TT_FILL_0_NORMAL+512)*ENTRY_SIZE
.global fill_0_normal_high
fill_0_normal_high:
	FILL_NORMAL_HANDLER


/* Preemptible trap handler.
 *
 * This trap handler makes arrangements to
 * make calling scheduler() possible.
 *
 * The caller is responsible for doing save
 * and allocating PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE
 * bytes on stack.
 *
 * Input registers:
 * 	%l0	 	Address of function to call.
 * Output registers:
 *	 %l1 - %l7 	Copy of %g1 - %g7
 */
.global preemptible_handler
preemptible_handler:
	/*
	 * Save TSTATE, TPC, TNPC and PSTATE aside.
	 */
	rdpr %tstate, %g1
	rdpr %tpc, %g2
	rdpr %tnpc, %g3
	rdpr %pstate, %g4

	stx %g1, [%fp + STACK_BIAS + SAVED_TSTATE]
	stx %g2, [%fp + STACK_BIAS + SAVED_TPC]
	stx %g3, [%fp + STACK_BIAS + SAVED_TNPC]
	stx %g4, [%fp + STACK_BIAS + SAVED_PSTATE]
	
	/*
	 * Write 0 to TL.
	 */
	wrpr %g0, 0, %tl
	
	/*
	 * Alter PSTATE.
	 * - switch to normal globals.
	 */
	and %g4, ~1, %g4		! mask alternate globals
	wrpr %g4, 0, %pstate
	 
	/*
	 * Save the normal globals.
	 */
	SAVE_GLOBALS
	
	/*
	 * Call the higher-level handler.
	 */
	call %l0
	nop
	
	/*
	 * Restore the normal global register set.
	 */
	RESTORE_GLOBALS
	
	/*
	 * Restore PSTATE from saved copy.
	 * Alternate globals become active.
	 */
	ldx [%fp + STACK_BIAS + SAVED_PSTATE], %l4
	wrpr %l4, 0, %pstate
	
	/*
	 * Write 1 to TL.
	 */
	wrpr %g0, 1, %tl
	
	/*
	 * Read TSTATE, TPC and TNPC from saved copy.
	 */
	ldx [%fp + STACK_BIAS + SAVED_TSTATE], %g1
	ldx [%fp + STACK_BIAS + SAVED_TPC], %g2
	ldx [%fp + STACK_BIAS + SAVED_TNPC], %g3

	/*
	 * Do restore to match the save instruction from the top-level handler.
	 */
	restore

	/*
	 * On execution of retry instruction, CWP will be restored from TSTATE register.
	 * However, because of scheduling, it is possible that CWP in saved TSTATE
	 * is different from current CWP. The following chunk of code fixes CWP
	 * in the saved copy of TSTATE.
	 */
	rdpr %cwp, %g4		! read current CWP
	and %g1, ~0x1f, %g1	! clear CWP field in saved TSTATE
	or %g1, %g4, %g1	! write current CWP to TSTATE
	
	/*
	 * Restore TSTATE, TPC and TNPC from saved copies.
	 */
	wrpr %g1, 0, %tstate
	wrpr %g2, 0, %tpc
	wrpr %g3, 0, %tnpc
	 
	/*
	 * Return from interrupt.
	 */
	retry
