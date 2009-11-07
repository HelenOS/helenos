#
# Copyright (c) 2005 Jakub Jermar
# Copyright (c) 2008 Pavel Rimsky
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
 * @file
 * @brief This file contains kernel trap table.
 */

.register %g2, #scratch
.register %g3, #scratch

.text

#include <arch/trap/trap_table.h>
#include <arch/trap/regwin.h>
#include <arch/trap/interrupt.h>
#include <arch/trap/exception.h>
#include <arch/trap/syscall.h>
#include <arch/trap/sun4v/mmu.h>
#include <arch/mm/sun4v/mmu.h>
#include <arch/mm/page.h>
#include <arch/stack.h>
#include <arch/sun4v/regdef.h>

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
.global instruction_access_exception_tl0
instruction_access_exception_tl0:
	/*wrpr %g0, PSTATE_AG_BIT | PSTATE_PRIV_BIT, %pstate
	PREEMPTIBLE_HANDLER instruction_access_exception*/

/* TT = 0x0a, TL = 0, instruction_access_error */
.org trap_table + TT_INSTRUCTION_ACCESS_ERROR*ENTRY_SIZE
.global instruction_access_error_tl0
instruction_access_error_tl0:
	PREEMPTIBLE_HANDLER instruction_access_error

/* TT = 0x10, TL = 0, illegal_instruction */
.org trap_table + TT_ILLEGAL_INSTRUCTION*ENTRY_SIZE
.global illegal_instruction_tl0
illegal_instruction_tl0:
	PREEMPTIBLE_HANDLER illegal_instruction

/* TT = 0x11, TL = 0, privileged_opcode */
.org trap_table + TT_PRIVILEGED_OPCODE*ENTRY_SIZE
.global privileged_opcode_tl0
privileged_opcode_tl0:
	PREEMPTIBLE_HANDLER privileged_opcode

/* TT = 0x12, TL = 0, unimplemented_LDD */
.org trap_table + TT_UNIMPLEMENTED_LDD*ENTRY_SIZE
.global unimplemented_LDD_tl0
unimplemented_LDD_tl0:
	PREEMPTIBLE_HANDLER unimplemented_LDD

/* TT = 0x13, TL = 0, unimplemented_STD */
.org trap_table + TT_UNIMPLEMENTED_STD*ENTRY_SIZE
.global unimplemented_STD_tl0
unimplemented_STD_tl0:
	PREEMPTIBLE_HANDLER unimplemented_STD

/* TT = 0x20, TL = 0, fb_disabled handler */
.org trap_table + TT_FP_DISABLED*ENTRY_SIZE
.global fb_disabled_tl0
fp_disabled_tl0:
	PREEMPTIBLE_HANDLER fp_disabled

/* TT = 0x21, TL = 0, fb_exception_ieee_754 handler */
.org trap_table + TT_FP_EXCEPTION_IEEE_754*ENTRY_SIZE
.global fb_exception_ieee_754_tl0
fp_exception_ieee_754_tl0:
	PREEMPTIBLE_HANDLER fp_exception_ieee_754

/* TT = 0x22, TL = 0, fb_exception_other handler */
.org trap_table + TT_FP_EXCEPTION_OTHER*ENTRY_SIZE
.global fb_exception_other_tl0
fp_exception_other_tl0:
	PREEMPTIBLE_HANDLER fp_exception_other

/* TT = 0x23, TL = 0, tag_overflow */
.org trap_table + TT_TAG_OVERFLOW*ENTRY_SIZE
.global tag_overflow_tl0
tag_overflow_tl0:
	PREEMPTIBLE_HANDLER tag_overflow

/* TT = 0x24, TL = 0, clean_window handler */
.org trap_table + TT_CLEAN_WINDOW*ENTRY_SIZE
.global clean_window_tl0
clean_window_tl0:
	CLEAN_WINDOW_HANDLER

/* TT = 0x28, TL = 0, division_by_zero */
.org trap_table + TT_DIVISION_BY_ZERO*ENTRY_SIZE
.global division_by_zero_tl0
division_by_zero_tl0:
	PREEMPTIBLE_HANDLER division_by_zero

/* TT = 0x30, TL = 0, data_access_exception */
.org trap_table + TT_DATA_ACCESS_EXCEPTION*ENTRY_SIZE
.global data_access_exception_tl0
data_access_exception_tl0:
	/*wrpr %g0, PSTATE_AG_BIT | PSTATE_PRIV_BIT, %pstate
	PREEMPTIBLE_HANDLER data_access_exception*/

/* TT = 0x32, TL = 0, data_access_error */
.org trap_table + TT_DATA_ACCESS_ERROR*ENTRY_SIZE
.global data_access_error_tl0
data_access_error_tl0:
	PREEMPTIBLE_HANDLER data_access_error

/* TT = 0x34, TL = 0, mem_address_not_aligned */
.org trap_table + TT_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global mem_address_not_aligned_tl0
mem_address_not_aligned_tl0:
	PREEMPTIBLE_HANDLER mem_address_not_aligned

/* TT = 0x35, TL = 0, LDDF_mem_address_not_aligned */
.org trap_table + TT_LDDF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global LDDF_mem_address_not_aligned_tl0
LDDF_mem_address_not_aligned_tl0:
	PREEMPTIBLE_HANDLER LDDF_mem_address_not_aligned

/* TT = 0x36, TL = 0, STDF_mem_address_not_aligned */
.org trap_table + TT_STDF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global STDF_mem_address_not_aligned_tl0
STDF_mem_address_not_aligned_tl0:
	PREEMPTIBLE_HANDLER STDF_mem_address_not_aligned

/* TT = 0x37, TL = 0, privileged_action */
.org trap_table + TT_PRIVILEGED_ACTION*ENTRY_SIZE
.global privileged_action_tl0
privileged_action_tl0:
	PREEMPTIBLE_HANDLER privileged_action

/* TT = 0x38, TL = 0, LDQF_mem_address_not_aligned */
.org trap_table + TT_LDQF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global LDQF_mem_address_not_aligned_tl0
LDQF_mem_address_not_aligned_tl0:
	PREEMPTIBLE_HANDLER LDQF_mem_address_not_aligned

/* TT = 0x39, TL = 0, STQF_mem_address_not_aligned */
.org trap_table + TT_STQF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
.global STQF_mem_address_not_aligned_tl0
STQF_mem_address_not_aligned_tl0:
	PREEMPTIBLE_HANDLER STQF_mem_address_not_aligned

/* TT = 0x41, TL = 0, interrupt_level_1 handler */
.org trap_table + TT_INTERRUPT_LEVEL_1*ENTRY_SIZE
.global interrupt_level_1_handler_tl0
interrupt_level_1_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 1

/* TT = 0x42, TL = 0, interrupt_level_2 handler */
.org trap_table + TT_INTERRUPT_LEVEL_2*ENTRY_SIZE
.global interrupt_level_2_handler_tl0
interrupt_level_2_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 2

/* TT = 0x43, TL = 0, interrupt_level_3 handler */
.org trap_table + TT_INTERRUPT_LEVEL_3*ENTRY_SIZE
.global interrupt_level_3_handler_tl0
interrupt_level_3_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 3

/* TT = 0x44, TL = 0, interrupt_level_4 handler */
.org trap_table + TT_INTERRUPT_LEVEL_4*ENTRY_SIZE
.global interrupt_level_4_handler_tl0
interrupt_level_4_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 4

/* TT = 0x45, TL = 0, interrupt_level_5 handler */
.org trap_table + TT_INTERRUPT_LEVEL_5*ENTRY_SIZE
.global interrupt_level_5_handler_tl0
interrupt_level_5_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 5

/* TT = 0x46, TL = 0, interrupt_level_6 handler */
.org trap_table + TT_INTERRUPT_LEVEL_6*ENTRY_SIZE
.global interrupt_level_6_handler_tl0
interrupt_level_6_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 6

/* TT = 0x47, TL = 0, interrupt_level_7 handler */
.org trap_table + TT_INTERRUPT_LEVEL_7*ENTRY_SIZE
.global interrupt_level_7_handler_tl0
interrupt_level_7_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 7

/* TT = 0x48, TL = 0, interrupt_level_8 handler */
.org trap_table + TT_INTERRUPT_LEVEL_8*ENTRY_SIZE
.global interrupt_level_8_handler_tl0
interrupt_level_8_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 8

/* TT = 0x49, TL = 0, interrupt_level_9 handler */
.org trap_table + TT_INTERRUPT_LEVEL_9*ENTRY_SIZE
.global interrupt_level_9_handler_tl0
interrupt_level_9_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 9

/* TT = 0x4a, TL = 0, interrupt_level_10 handler */
.org trap_table + TT_INTERRUPT_LEVEL_10*ENTRY_SIZE
.global interrupt_level_10_handler_tl0
interrupt_level_10_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 10

/* TT = 0x4b, TL = 0, interrupt_level_11 handler */
.org trap_table + TT_INTERRUPT_LEVEL_11*ENTRY_SIZE
.global interrupt_level_11_handler_tl0
interrupt_level_11_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 11

/* TT = 0x4c, TL = 0, interrupt_level_12 handler */
.org trap_table + TT_INTERRUPT_LEVEL_12*ENTRY_SIZE
.global interrupt_level_12_handler_tl0
interrupt_level_12_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 12

/* TT = 0x4d, TL = 0, interrupt_level_13 handler */
.org trap_table + TT_INTERRUPT_LEVEL_13*ENTRY_SIZE
.global interrupt_level_13_handler_tl0
interrupt_level_13_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 13

/* TT = 0x4e, TL = 0, interrupt_level_14 handler */
.org trap_table + TT_INTERRUPT_LEVEL_14*ENTRY_SIZE
.global interrupt_level_14_handler_tl0
interrupt_level_14_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 14

/* TT = 0x4f, TL = 0, interrupt_level_15 handler */
.org trap_table + TT_INTERRUPT_LEVEL_15*ENTRY_SIZE
.global interrupt_level_15_handler_tl0
interrupt_level_15_handler_tl0:
	INTERRUPT_LEVEL_N_HANDLER 15

/* TT = 0x60, TL = 0, interrupt_vector_trap handler */
.org trap_table + TT_INTERRUPT_VECTOR_TRAP*ENTRY_SIZE
.global interrupt_vector_trap_handler_tl0
interrupt_vector_trap_handler_tl0:
	INTERRUPT_VECTOR_TRAP_HANDLER

/* TT = 0x64, TL = 0, fast_instruction_access_MMU_miss */
.org trap_table + TT_FAST_INSTRUCTION_ACCESS_MMU_MISS*ENTRY_SIZE
.global fast_instruction_access_mmu_miss_handler_tl0
fast_instruction_access_mmu_miss_handler_tl0:
	/*FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER*/

/* TT = 0x68, TL = 0, fast_data_access_MMU_miss */
.org trap_table + TT_FAST_DATA_ACCESS_MMU_MISS*ENTRY_SIZE
.global fast_data_access_mmu_miss_handler_tl0
fast_data_access_mmu_miss_handler_tl0:
	FAST_DATA_ACCESS_MMU_MISS_HANDLER 0

/* TT = 0x6c, TL = 0, fast_data_access_protection */
.org trap_table + TT_FAST_DATA_ACCESS_PROTECTION*ENTRY_SIZE
.global fast_data_access_protection_handler_tl0
fast_data_access_protection_handler_tl0:
	/*FAST_DATA_ACCESS_PROTECTION_HANDLER 0*/

/* TT = 0x80, TL = 0, spill_0_normal handler */
.org trap_table + TT_SPILL_0_NORMAL*ENTRY_SIZE
.global spill_0_normal_tl0
spill_0_normal_tl0:
	SPILL_NORMAL_HANDLER_KERNEL

/* TT = 0x84, TL = 0, spill_1_normal handler */
.org trap_table + TT_SPILL_1_NORMAL*ENTRY_SIZE
.global spill_1_normal_tl0
spill_1_normal_tl0:
	SPILL_NORMAL_HANDLER_USERSPACE

/* TT = 0x88, TL = 0, spill_2_normal handler */
.org trap_table + TT_SPILL_2_NORMAL*ENTRY_SIZE
.global spill_2_normal_tl0
spill_2_normal_tl0:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xa0, TL = 0, spill_0_other handler */
.org trap_table + TT_SPILL_0_OTHER*ENTRY_SIZE
.global spill_0_other_tl0
spill_0_other_tl0:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xc0, TL = 0, fill_0_normal handler */
.org trap_table + TT_FILL_0_NORMAL*ENTRY_SIZE
.global fill_0_normal_tl0
fill_0_normal_tl0:
	FILL_NORMAL_HANDLER_KERNEL

/* TT = 0xc4, TL = 0, fill_1_normal handler */
.org trap_table + TT_FILL_1_NORMAL*ENTRY_SIZE
.global fill_1_normal_tl0
fill_1_normal_tl0:
	FILL_NORMAL_HANDLER_USERSPACE

/* TT = 0x100 - 0x17f, TL = 0, trap_instruction_0 - trap_instruction_7f */
.irp cur, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,\
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,\
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,\
    58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,\
    77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,\
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,\
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,\
    127
.org trap_table + (TT_TRAP_INSTRUCTION_0+\cur)*ENTRY_SIZE
.global trap_instruction_\cur\()_tl0
trap_instruction_\cur\()_tl0:
	ba trap_instruction_handler
	mov \cur, %g2
.endr

/*
 * Handlers for TL>0.
 */

/* TT = 0x08, TL > 0, instruction_access_exception */
.org trap_table + (TT_INSTRUCTION_ACCESS_EXCEPTION+512)*ENTRY_SIZE
.global instruction_access_exception_tl1
instruction_access_exception_tl1:
	/*wrpr %g0, 1, %tl
	wrpr %g0, PSTATE_AG_BIT | PSTATE_PRIV_BIT, %pstate
	PREEMPTIBLE_HANDLER instruction_access_exception*/

/* TT = 0x0a, TL > 0, instruction_access_error */
.org trap_table + (TT_INSTRUCTION_ACCESS_ERROR+512)*ENTRY_SIZE
.global instruction_access_error_tl1
instruction_access_error_tl1:
	wrpr %g0, 1, %tl
	PREEMPTIBLE_HANDLER instruction_access_error

/* TT = 0x10, TL > 0, illegal_instruction */
.org trap_table + (TT_ILLEGAL_INSTRUCTION+512)*ENTRY_SIZE
.global illegal_instruction_tl1
illegal_instruction_tl1:
	wrpr %g0, 1, %tl
	PREEMPTIBLE_HANDLER illegal_instruction

/* TT = 0x24, TL > 0, clean_window handler */
.org trap_table + (TT_CLEAN_WINDOW+512)*ENTRY_SIZE
.global clean_window_tl1
clean_window_tl1:
	CLEAN_WINDOW_HANDLER

/* TT = 0x28, TL > 0, division_by_zero */
.org trap_table + (TT_DIVISION_BY_ZERO+512)*ENTRY_SIZE
.global division_by_zero_tl1
division_by_zero_tl1:
	wrpr %g0, 1, %tl
	PREEMPTIBLE_HANDLER division_by_zero

/* TT = 0x30, TL > 0, data_access_exception */
.org trap_table + (TT_DATA_ACCESS_EXCEPTION+512)*ENTRY_SIZE
.global data_access_exception_tl1
data_access_exception_tl1:
	/*wrpr %g0, 1, %tl
	wrpr %g0, PSTATE_AG_BIT | PSTATE_PRIV_BIT, %pstate
	PREEMPTIBLE_HANDLER data_access_exception*/

/* TT = 0x32, TL > 0, data_access_error */
.org trap_table + (TT_DATA_ACCESS_ERROR+512)*ENTRY_SIZE
.global data_access_error_tl1
data_access_error_tl1:
	wrpr %g0, 1, %tl
	PREEMPTIBLE_HANDLER data_access_error

/* TT = 0x34, TL > 0, mem_address_not_aligned */
.org trap_table + (TT_MEM_ADDRESS_NOT_ALIGNED+512)*ENTRY_SIZE
.global mem_address_not_aligned_tl1
mem_address_not_aligned_tl1:
	wrpr %g0, 1, %tl
	PREEMPTIBLE_HANDLER mem_address_not_aligned

/* TT = 0x68, TL > 0, fast_data_access_MMU_miss */
.org trap_table + (TT_FAST_DATA_ACCESS_MMU_MISS+512)*ENTRY_SIZE
.global fast_data_access_mmu_miss_handler_tl1
fast_data_access_mmu_miss_handler_tl1:
	FAST_DATA_ACCESS_MMU_MISS_HANDLER 1

/* TT = 0x6c, TL > 0, fast_data_access_protection */
.org trap_table + (TT_FAST_DATA_ACCESS_PROTECTION+512)*ENTRY_SIZE
.global fast_data_access_protection_handler_tl1
fast_data_access_protection_handler_tl1:
	/*FAST_DATA_ACCESS_PROTECTION_HANDLER 1*/

/* TT = 0x80, TL > 0, spill_0_normal handler */
.org trap_table + (TT_SPILL_0_NORMAL+512)*ENTRY_SIZE
.global spill_0_normal_tl1
spill_0_normal_tl1:
	SPILL_NORMAL_HANDLER_KERNEL

/* TT = 0x88, TL > 0, spill_2_normal handler */
.org trap_table + (TT_SPILL_2_NORMAL+512)*ENTRY_SIZE
.global spill_2_normal_tl1
spill_2_normal_tl1:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xa0, TL > 0, spill_0_other handler */
.org trap_table + (TT_SPILL_0_OTHER+512)*ENTRY_SIZE
.global spill_0_other_tl1
spill_0_other_tl1:
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xc0, TL > 0, fill_0_normal handler */
.org trap_table + (TT_FILL_0_NORMAL+512)*ENTRY_SIZE
.global fill_0_normal_tl1
fill_0_normal_tl1:
	FILL_NORMAL_HANDLER_KERNEL

.align TABLE_SIZE


/*
 * Spills the window at CWP + 2 to the kernel stack. This macro is to be
 * used before doing SAVE when the spill trap is undesirable.
 * 
 * Parameters:
 * 	tmpreg1		global register to be used for scratching purposes
 * 	tmpreg2		global register to be used for scratching purposes
 */
.macro INLINE_SPILL tmpreg1, tmpreg2
	! CWP := CWP + 2
	rdpr %cwp, \tmpreg2
	add \tmpreg2, 2, \tmpreg1
	and \tmpreg1, NWINDOWS - 1, \tmpreg1		! modulo NWINDOWS
	wrpr \tmpreg1, %cwp
	
	! spill to kernel stack
	stx %l0, [%sp + STACK_BIAS + L0_OFFSET]	
	stx %l1, [%sp + STACK_BIAS + L1_OFFSET]
	stx %l2, [%sp + STACK_BIAS + L2_OFFSET]
	stx %l3, [%sp + STACK_BIAS + L3_OFFSET]
	stx %l4, [%sp + STACK_BIAS + L4_OFFSET]
	stx %l5, [%sp + STACK_BIAS + L5_OFFSET]
	stx %l6, [%sp + STACK_BIAS + L6_OFFSET]
	stx %l7, [%sp + STACK_BIAS + L7_OFFSET]
	stx %i0, [%sp + STACK_BIAS + I0_OFFSET]
	stx %i1, [%sp + STACK_BIAS + I1_OFFSET]
	stx %i2, [%sp + STACK_BIAS + I2_OFFSET]
	stx %i3, [%sp + STACK_BIAS + I3_OFFSET]
	stx %i4, [%sp + STACK_BIAS + I4_OFFSET]
	stx %i5, [%sp + STACK_BIAS + I5_OFFSET]
	stx %i6, [%sp + STACK_BIAS + I6_OFFSET]
	stx %i7, [%sp + STACK_BIAS + I7_OFFSET]

	! CWP := CWP - 2
	wrpr \tmpreg2, %cwp

	saved
.endm

/*
 * Fill the window at CWP - 1 from the kernel stack. This macro is to be
 * used before doing RESTORE when the fill trap is undesirable.
 * 
 * Parameters:
 * 	tmpreg1		global register to be used for scratching purposes
 * 	tmpreg2		global register to be used for scratching purposes
 */
.macro INLINE_FILL tmpreg1, tmpreg2
	! CWP := CWP - 1
	rdpr %cwp, \tmpreg2
	add \tmpreg2, NWINDOWS - 1, \tmpreg1
	and \tmpreg1, NWINDOWS - 1, \tmpreg1
	wrpr \tmpreg1, %cwp

	! fill from kernel stack
	ldx [%sp + STACK_BIAS + L0_OFFSET], %l0
	ldx [%sp + STACK_BIAS + L1_OFFSET], %l1
	ldx [%sp + STACK_BIAS + L2_OFFSET], %l2
	ldx [%sp + STACK_BIAS + L3_OFFSET], %l3
	ldx [%sp + STACK_BIAS + L4_OFFSET], %l4
	ldx [%sp + STACK_BIAS + L5_OFFSET], %l5
	ldx [%sp + STACK_BIAS + L6_OFFSET], %l6
	ldx [%sp + STACK_BIAS + L7_OFFSET], %l7
	ldx [%sp + STACK_BIAS + I0_OFFSET], %i0
	ldx [%sp + STACK_BIAS + I1_OFFSET], %i1
	ldx [%sp + STACK_BIAS + I2_OFFSET], %i2
	ldx [%sp + STACK_BIAS + I3_OFFSET], %i3
	ldx [%sp + STACK_BIAS + I4_OFFSET], %i4
	ldx [%sp + STACK_BIAS + I5_OFFSET], %i5
	ldx [%sp + STACK_BIAS + I6_OFFSET], %i6
	ldx [%sp + STACK_BIAS + I7_OFFSET], %i7

	! CWP := CWP + 1
	wrpr \tmpreg2, %cwp

	restored
.endm

/*
 * Preemptible trap handler for handling traps from kernel.
 */
.macro PREEMPTIBLE_HANDLER_KERNEL

	/*
	 * ASSERT(%tl == 1)
	 */
	rdpr %tl, %g3
	cmp %g3, 1
	be 1f
	nop
0:	ba 0b					! this is for debugging, if we ever get here
	nop					! it will be easy to find

	/* prevent unnecessary CLEANWIN exceptions */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(0), %wstate
1:
	/*
	 * Prevent SAVE instruction from causing a spill exception. If the
	 * CANSAVE register is zero, explicitly spill register window
	 * at CWP + 2.
	 */

	rdpr %cansave, %g3
	brnz %g3, 2f
	nop
	INLINE_SPILL %g3, %g4

2:
	/* ask for new register window */
	save %sp, -PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE, %sp

	/* copy higher level routine's address and its argument */
	mov %g1, %l0
	mov %g2, %o0

	/*
	 * Save TSTATE, TPC and TNPC aside.
	 */
	rdpr %tstate, %g1
	rdpr %tpc, %g2
	rdpr %tnpc, %g3

	stx %g1, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TSTATE]
	stx %g2, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TPC]
	stx %g3, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TNPC]

	/*
	 * Save the Y register.
	 * This register is deprecated according to SPARC V9 specification
	 * and is only present for backward compatibility with previous
	 * versions of the SPARC architecture.
	 * Surprisingly, gcc makes use of this register without a notice.
	 */
	rd %y, %g4
	stx %g4, [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_Y]

	/* switch to TL = 0, explicitly enable FPU */
	wrpr %g0, 0, %tl
	wrpr %g0, 0, %gl
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_PEF_BIT, %pstate

	/* g1 -> l1, ..., g7 -> l7 */
	SAVE_GLOBALS

	/* call higher-level service routine, pass istate as its 2nd parameter */
	call %l0
	add %sp, PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TNPC, %o1

	/* l1 -> g1, ..., l7 -> g7 */
	RESTORE_GLOBALS

	/* we must prserve the PEF bit */
	rdpr %pstate, %l1

	/* TL := 1, GL := 1 */
	wrpr %g0, PSTATE_PRIV_BIT, %pstate
	wrpr %g0, 1, %tl
	wrpr %g0, 1, %gl

	/* Read TSTATE, TPC and TNPC from saved copy. */
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TSTATE], %g1
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TPC], %g2
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_TNPC], %g3

	/* Copy PSTATE.PEF to the in-register copy of TSTATE. */
	and %l1, PSTATE_PEF_BIT, %l1
	sllx %l1, TSTATE_PSTATE_SHIFT, %l1
	sethi %hi(TSTATE_PEF_BIT), %g4		! reset the PEF bit to 0 ...
	andn %g1, %g4, %g1
	or %g1, %l1, %g1			! ... "or" it with saved PEF

	/* Restore TSTATE, TPC and TNPC from saved copies. */
	wrpr %g1, 0, %tstate
	wrpr %g2, 0, %tpc
	wrpr %g3, 0, %tnpc

	/* Restore Y. */
	ldx [%sp + PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE + STACK_BIAS + SAVED_Y], %g4
	wr %g4, %y
	
	/* If TSTATE.CWP + 1 == CWP, then we do not have to fix CWP. */
	and %g1, TSTATE_CWP_MASK, %l0
	inc %l0
	and %l0, NWINDOWS - 1, %l0	! %l0 mod NWINDOWS
	rdpr %cwp, %l1
	cmp %l0, %l1
	bz 4f				! CWP is ok
	nop

3:
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

4:
	/*
	 * Prevent RESTORE instruction from causing a fill exception. If the
	 * CANRESTORE register is zero, explicitly fill register window
	 * at CWP - 1.
	 */
	rdpr %canrestore, %g1
	brnz %g1, 5f
	nop
	INLINE_FILL %g3, %g4

5:
	restore

	retry
.endm


#define NOT(x)	((x) == 0)

/* Preemptible trap handler for TL=1.
 *
 * This trap handler makes arrangements to make calling of scheduler() from
 * within a trap context possible. It is called from several other trap
 * handlers.
 */
.macro PREEMPTIBLE_HANDLER_TEMPLATE is_syscall
	PREEMPTIBLE_HANDLER_KERNEL
.endm

.global preemptible_handler
preemptible_handler:
	PREEMPTIBLE_HANDLER_TEMPLATE 0

.global trap_instruction_handler
trap_instruction_handler:
	PREEMPTIBLE_HANDLER_TEMPLATE 1

