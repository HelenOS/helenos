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

#include <abi/asmtool.h>
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
#include <arch/sun4v/arch.h>
#include <arch/sun4v/cpu.h>

#define TABLE_SIZE	TRAP_TABLE_SIZE
#define ENTRY_SIZE	TRAP_TABLE_ENTRY_SIZE

/*
 * Kernel trap table.
 */
.align TABLE_SIZE
SYMBOL(trap_table)

/* TT = 0x08, TL = 0, instruction_access_exception */
/* TT = 0x08, TL = 0, IAE_privilege_violation on UltraSPARC T2 */
.org trap_table + TT_INSTRUCTION_ACCESS_EXCEPTION*ENTRY_SIZE
SYMBOL(instruction_access_exception_tl0)
	mov TT_INSTRUCTION_ACCESS_EXCEPTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x09, TL = 0, instruction_access_mmu_miss */
.org trap_table + TT_INSTRUCTION_ACCESS_MMU_MISS*ENTRY_SIZE
SYMBOL(instruction_access_mmu_miss_handler_tl0)
	ba,a %xcc, fast_instruction_access_mmu_miss_handler_tl0

/* TT = 0x0a, TL = 0, instruction_access_error */
.org trap_table + TT_INSTRUCTION_ACCESS_ERROR*ENTRY_SIZE
SYMBOL(instruction_access_error_tl0)
	mov TT_INSTRUCTION_ACCESS_ERROR, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x0b, TL = 0, IAE_unauth_access */
.org trap_table + TT_IAE_UNAUTH_ACCESS*ENTRY_SIZE
SYMBOL(iae_unauth_access_tl0)
	mov TT_IAE_UNAUTH_ACCESS, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x0c, TL = 0, IAE_nfo_page */
.org trap_table + TT_IAE_NFO_PAGE*ENTRY_SIZE
SYMBOL(iae_nfo_page_tl0)
	mov TT_IAE_NFO_PAGE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x10, TL = 0, illegal_instruction */
.org trap_table + TT_ILLEGAL_INSTRUCTION*ENTRY_SIZE
SYMBOL(illegal_instruction_tl0)
	mov TT_ILLEGAL_INSTRUCTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x11, TL = 0, privileged_opcode */
.org trap_table + TT_PRIVILEGED_OPCODE*ENTRY_SIZE
SYMBOL(privileged_opcode_tl0)
	mov TT_PRIVILEGED_OPCODE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x12, TL = 0, unimplemented_LDD */
.org trap_table + TT_UNIMPLEMENTED_LDD*ENTRY_SIZE
SYMBOL(unimplemented_LDD_tl0)
	mov TT_UNIMPLEMENTED_LDD, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x13, TL = 0, unimplemented_STD */
.org trap_table + TT_UNIMPLEMENTED_STD*ENTRY_SIZE
SYMBOL(unimplemented_STD_tl0)
	mov TT_UNIMPLEMENTED_STD, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x14, TL = 0, DAE_invalid_asi */
.org trap_table + TT_DAE_INVALID_ASI*ENTRY_SIZE
SYMBOL(dae_invalid_asi_tl0)
	mov TT_DAE_INVALID_ASI, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x15, TL = 0, DAE_privilege_violation */
.org trap_table + TT_DAE_PRIVILEGE_VIOLATION*ENTRY_SIZE
SYMBOL(dae_privilege_violation_tl0)
	mov TT_DAE_PRIVILEGE_VIOLATION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x16, TL = 0, DAE_nc_page */
.org trap_table + TT_DAE_NC_PAGE*ENTRY_SIZE
SYMBOL(dae_nc_page_tl0)
	mov TT_DAE_NC_PAGE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x17, TL = 0, DAE_nfo_page */
.org trap_table + TT_DAE_NFO_PAGE*ENTRY_SIZE
SYMBOL(dae_nfo_page_tl0)
	mov TT_DAE_NFO_PAGE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x20, TL = 0, fb_disabled handler */
.org trap_table + TT_FP_DISABLED*ENTRY_SIZE
SYMBOL(fp_disabled_tl0)
	mov TT_FP_DISABLED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x21, TL = 0, fb_exception_ieee_754 handler */
.org trap_table + TT_FP_EXCEPTION_IEEE_754*ENTRY_SIZE
SYMBOL(fp_exception_ieee_754_tl0)
	mov TT_FP_EXCEPTION_IEEE_754, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x22, TL = 0, fb_exception_other handler */
.org trap_table + TT_FP_EXCEPTION_OTHER*ENTRY_SIZE
SYMBOL(fp_exception_other_tl)
	mov TT_FP_EXCEPTION_OTHER, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x23, TL = 0, tag_overflow */
.org trap_table + TT_TAG_OVERFLOW*ENTRY_SIZE
SYMBOL(tag_overflow_tl0)
	mov TT_TAG_OVERFLOW, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x24, TL = 0, clean_window handler */
.org trap_table + TT_CLEAN_WINDOW*ENTRY_SIZE
SYMBOL(clean_window_tl0)
	CLEAN_WINDOW_HANDLER

/* TT = 0x28, TL = 0, division_by_zero */
.org trap_table + TT_DIVISION_BY_ZERO*ENTRY_SIZE
SYMBOL(division_by_zero_tl0)
	mov TT_DIVISION_BY_ZERO, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x30, TL = 0, data_access_exception */
/* TT = 0x30, TL = 0, DAE_side_effect_page for UltraPSARC T2 */
.org trap_table + TT_DATA_ACCESS_EXCEPTION*ENTRY_SIZE
SYMBOL(data_access_exception_tl0)
	mov TT_DATA_ACCESS_EXCEPTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x31, TL = 0, data_access_mmu_miss */
.org trap_table + TT_DATA_ACCESS_MMU_MISS*ENTRY_SIZE
SYMBOL(data_access_mmu_miss_tl0)
	ba,a %xcc, fast_data_access_mmu_miss_handler_tl0

/* TT = 0x32, TL = 0, data_access_error */
.org trap_table + TT_DATA_ACCESS_ERROR*ENTRY_SIZE
SYMBOL(data_access_error_tl0)
	mov TT_DATA_ACCESS_ERROR, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x34, TL = 0, mem_address_not_aligned */
.org trap_table + TT_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
SYMBOL(mem_address_not_aligned_tl0)
	mov TT_MEM_ADDRESS_NOT_ALIGNED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x35, TL = 0, LDDF_mem_address_not_aligned */
.org trap_table + TT_LDDF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
SYMBOL(LDDF_mem_address_not_aligned_tl0)
	mov TT_LDDF_MEM_ADDRESS_NOT_ALIGNED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x36, TL = 0, STDF_mem_address_not_aligned */
.org trap_table + TT_STDF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
SYMBOL(STDF_mem_address_not_aligned_tl0)
	mov TT_STDF_MEM_ADDRESS_NOT_ALIGNED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x37, TL = 0, privileged_action */
.org trap_table + TT_PRIVILEGED_ACTION*ENTRY_SIZE
SYMBOL(privileged_action_tl0)
	mov TT_PRIVILEGED_ACTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x38, TL = 0, LDQF_mem_address_not_aligned */
.org trap_table + TT_LDQF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
SYMBOL(LDQF_mem_address_not_aligned_tl0)
	mov TT_LDQF_MEM_ADDRESS_NOT_ALIGNED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x39, TL = 0, STQF_mem_address_not_aligned */
.org trap_table + TT_STQF_MEM_ADDRESS_NOT_ALIGNED*ENTRY_SIZE
SYMBOL(STQF_mem_address_not_aligned_tl0)
	mov TT_STQF_MEM_ADDRESS_NOT_ALIGNED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x41, TL = 0, interrupt_level_1 handler */
.org trap_table + TT_INTERRUPT_LEVEL_1*ENTRY_SIZE
SYMBOL(interrupt_level_1_handler_tl0)
	mov TT_INTERRUPT_LEVEL_1, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x42, TL = 0, interrupt_level_2 handler */
.org trap_table + TT_INTERRUPT_LEVEL_2*ENTRY_SIZE
SYMBOL(interrupt_level_2_handler_tl0)
	mov TT_INTERRUPT_LEVEL_2, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x43, TL = 0, interrupt_level_3 handler */
.org trap_table + TT_INTERRUPT_LEVEL_3*ENTRY_SIZE
SYMBOL(interrupt_level_3_handler_tl0)
	mov TT_INTERRUPT_LEVEL_3, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x44, TL = 0, interrupt_level_4 handler */
.org trap_table + TT_INTERRUPT_LEVEL_4*ENTRY_SIZE
SYMBOL(interrupt_level_4_handler_tl0)
	mov TT_INTERRUPT_LEVEL_4, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x45, TL = 0, interrupt_level_5 handler */
.org trap_table + TT_INTERRUPT_LEVEL_5*ENTRY_SIZE
SYMBOL(interrupt_level_5_handler_tl0)
	mov TT_INTERRUPT_LEVEL_5, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x46, TL = 0, interrupt_level_6 handler */
.org trap_table + TT_INTERRUPT_LEVEL_6*ENTRY_SIZE
SYMBOL(interrupt_level_6_handler_tl0)
	mov TT_INTERRUPT_LEVEL_6, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x47, TL = 0, interrupt_level_7 handler */
.org trap_table + TT_INTERRUPT_LEVEL_7*ENTRY_SIZE
SYMBOL(interrupt_level_7_handler_tl0)
	mov TT_INTERRUPT_LEVEL_7, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x48, TL = 0, interrupt_level_8 handler */
.org trap_table + TT_INTERRUPT_LEVEL_8*ENTRY_SIZE
SYMBOL(interrupt_level_8_handler_tl0)
	mov TT_INTERRUPT_LEVEL_8, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x49, TL = 0, interrupt_level_9 handler */
.org trap_table + TT_INTERRUPT_LEVEL_9*ENTRY_SIZE
SYMBOL(interrupt_level_9_handler_tl0)
	mov TT_INTERRUPT_LEVEL_9, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x4a, TL = 0, interrupt_level_10 handler */
.org trap_table + TT_INTERRUPT_LEVEL_10*ENTRY_SIZE
SYMBOL(interrupt_level_10_handler_tl0)
	mov TT_INTERRUPT_LEVEL_10, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x4b, TL = 0, interrupt_level_11 handler */
.org trap_table + TT_INTERRUPT_LEVEL_11*ENTRY_SIZE
SYMBOL(interrupt_level_11_handler_tl0)
	mov TT_INTERRUPT_LEVEL_11, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x4c, TL = 0, interrupt_level_12 handler */
.org trap_table + TT_INTERRUPT_LEVEL_12*ENTRY_SIZE
SYMBOL(interrupt_level_12_handler_tl0)
	mov TT_INTERRUPT_LEVEL_12, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x4d, TL = 0, interrupt_level_13 handler */
.org trap_table + TT_INTERRUPT_LEVEL_13*ENTRY_SIZE
SYMBOL(interrupt_level_13_handler_tl0)
	mov TT_INTERRUPT_LEVEL_13, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x4e, TL = 0, interrupt_level_14 handler */
.org trap_table + TT_INTERRUPT_LEVEL_14*ENTRY_SIZE
SYMBOL(interrupt_level_14_handler_tl0)
	mov TT_INTERRUPT_LEVEL_14, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x4f, TL = 0, interrupt_level_15 handler */
.org trap_table + TT_INTERRUPT_LEVEL_15*ENTRY_SIZE
SYMBOL(interrupt_level_15_handler_tl0)
	mov TT_INTERRUPT_LEVEL_15, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x64, TL = 0, fast_instruction_access_MMU_miss */
.org trap_table + TT_FAST_INSTRUCTION_ACCESS_MMU_MISS*ENTRY_SIZE
SYMBOL(fast_instruction_access_mmu_miss_handler_tl0)
	FAST_INSTRUCTION_ACCESS_MMU_MISS_HANDLER

/* TT = 0x68, TL = 0, fast_data_access_MMU_miss */
.org trap_table + TT_FAST_DATA_ACCESS_MMU_MISS*ENTRY_SIZE
SYMBOL(fast_data_access_mmu_miss_handler_tl0)
	FAST_DATA_ACCESS_MMU_MISS_HANDLER 0

/* TT = 0x6c, TL = 0, fast_data_access_protection */
.org trap_table + TT_FAST_DATA_ACCESS_PROTECTION*ENTRY_SIZE
SYMBOL(fast_data_access_protection_handler_tl0)
	FAST_DATA_ACCESS_PROTECTION_HANDLER 0

/* TT = 0x7c, TL = 0, cpu_mondo */
.org trap_table + TT_CPU_MONDO*ENTRY_SIZE
SYMBOL(cpu_mondo_handler_tl0)
	mov TT_CPU_MONDO, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x80, TL = 0, spill_0_normal handler */
.org trap_table + TT_SPILL_0_NORMAL*ENTRY_SIZE
SYMBOL(spill_0_normal_tl0)
	SPILL_NORMAL_HANDLER_KERNEL

/* TT = 0x84, TL = 0, spill_1_normal handler */
.org trap_table + TT_SPILL_1_NORMAL*ENTRY_SIZE
SYMBOL(spill_1_normal_tl0)
	SPILL_NORMAL_HANDLER_USERSPACE

/* TT = 0x88, TL = 0, spill_2_normal handler */
.org trap_table + TT_SPILL_2_NORMAL*ENTRY_SIZE
SYMBOL(spill_2_normal_tl0)
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xa0, TL = 0, spill_0_other handler */
.org trap_table + TT_SPILL_0_OTHER*ENTRY_SIZE
SYMBOL(spill_0_other_tl0)
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xc0, TL = 0, fill_0_normal handler */
.org trap_table + TT_FILL_0_NORMAL*ENTRY_SIZE
SYMBOL(fill_0_normal_tl0)
	FILL_NORMAL_HANDLER_KERNEL

/* TT = 0xc4, TL = 0, fill_1_normal handler */
.org trap_table + TT_FILL_1_NORMAL*ENTRY_SIZE
SYMBOL(fill_1_normal_tl0)
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
SYMBOL(trap_instruction_\cur\()_tl0)
	mov \cur, %g2
	ba %xcc, trap_instruction_handler
	clr %g5
.endr

/*
 * Handlers for TL>0.
 */

/* TT = 0x08, TL > 0, instruction_access_exception */
/* TT = 0x08, TL > 0, IAE_privilege_violation on UltraSPARC T2 */
.org trap_table + (TT_INSTRUCTION_ACCESS_EXCEPTION+512)*ENTRY_SIZE
SYMBOL(instruction_access_exception_tl1)
	wrpr %g0, 1, %tl
	mov TT_INSTRUCTION_ACCESS_EXCEPTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x09, TL > 0, instruction_access_mmu_miss */
.org trap_table + (TT_INSTRUCTION_ACCESS_MMU_MISS+512)*ENTRY_SIZE
SYMBOL(instruction_access_mmu_miss_handler_tl1)
	wrpr %g0, 1, %tl
	ba,a %xcc, fast_instruction_access_mmu_miss_handler_tl0

/* TT = 0x0a, TL > 0, instruction_access_error */
.org trap_table + (TT_INSTRUCTION_ACCESS_ERROR+512)*ENTRY_SIZE
SYMBOL(instruction_access_error_tl1)
	wrpr %g0, 1, %tl
	mov TT_INSTRUCTION_ACCESS_ERROR, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x0b, TL > 0, IAE_unauth_access */
.org trap_table + (TT_IAE_UNAUTH_ACCESS+512)*ENTRY_SIZE
SYMBOL(iae_unauth_access_tl1)
	wrpr %g0, 1, %tl
	mov TT_IAE_UNAUTH_ACCESS, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x0c, TL > 0, IAE_nfo_page */
.org trap_table + (TT_IAE_NFO_PAGE+512)*ENTRY_SIZE
SYMBOL(iae_nfo_page_tl1)
	wrpr %g0, 1, %tl
	mov TT_IAE_NFO_PAGE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x10, TL > 0, illegal_instruction */
.org trap_table + (TT_ILLEGAL_INSTRUCTION+512)*ENTRY_SIZE
SYMBOL(illegal_instruction_tl1)
	wrpr %g0, 1, %tl
	mov TT_ILLEGAL_INSTRUCTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x14, TL > 0, DAE_invalid_asi */
.org trap_table + (TT_DAE_INVALID_ASI+512)*ENTRY_SIZE
SYMBOL(dae_invalid_asi_tl1)
	wrpr %g0, 1, %tl
	mov TT_DAE_INVALID_ASI, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x15, TL > 0, DAE_privilege_violation */
.org trap_table + (TT_DAE_PRIVILEGE_VIOLATION+512)*ENTRY_SIZE
SYMBOL(dae_privilege_violation_tl1)
	wrpr %g0, 1, %tl
	mov TT_DAE_PRIVILEGE_VIOLATION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x16, TL > 0, DAE_nc_page */
.org trap_table + (TT_DAE_NC_PAGE+512)*ENTRY_SIZE
SYMBOL(dae_nc_page_tl1)
	wrpr %g0, 1, %tl
	mov TT_DAE_NC_PAGE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x17, TL > 0, DAE_nfo_page */
.org trap_table + (TT_DAE_NFO_PAGE+512)*ENTRY_SIZE
SYMBOL(dae_nfo_page_tl1)
	wrpr %g0, 1, %tl
	mov TT_DAE_NFO_PAGE, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x24, TL > 0, clean_window handler */
.org trap_table + (TT_CLEAN_WINDOW+512)*ENTRY_SIZE
SYMBOL(clean_window_tl1)
	CLEAN_WINDOW_HANDLER

/* TT = 0x28, TL > 0, division_by_zero */
.org trap_table + (TT_DIVISION_BY_ZERO+512)*ENTRY_SIZE
SYMBOL(division_by_zero_tl1)
	wrpr %g0, 1, %tl
	mov TT_DIVISION_BY_ZERO, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x30, TL > 0, data_access_exception */
.org trap_table + (TT_DATA_ACCESS_EXCEPTION+512)*ENTRY_SIZE
SYMBOL(data_access_exception_tl1)
	wrpr %g0, 1, %tl
	mov TT_DATA_ACCESS_EXCEPTION, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x31, TL > 0, data_access_mmu_miss */
.org trap_table + (TT_DATA_ACCESS_MMU_MISS+512)*ENTRY_SIZE
SYMBOL(data_access_mmu_miss_tl1)
	ba,a %xcc, fast_data_access_mmu_miss_handler_tl1

/* TT = 0x32, TL > 0, data_access_error */
.org trap_table + (TT_DATA_ACCESS_ERROR+512)*ENTRY_SIZE
SYMBOL(data_access_error_tl1)
	wrpr %g0, 1, %tl
	mov TT_DATA_ACCESS_ERROR, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x34, TL > 0, mem_address_not_aligned */
.org trap_table + (TT_MEM_ADDRESS_NOT_ALIGNED+512)*ENTRY_SIZE
SYMBOL(mem_address_not_aligned_tl1)
	wrpr %g0, 1, %tl
	mov TT_MEM_ADDRESS_NOT_ALIGNED, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x68, TL > 0, fast_data_access_MMU_miss */
.org trap_table + (TT_FAST_DATA_ACCESS_MMU_MISS+512)*ENTRY_SIZE
SYMBOL(fast_data_access_mmu_miss_handler_tl1)
	FAST_DATA_ACCESS_MMU_MISS_HANDLER 1

/* TT = 0x6c, TL > 0, fast_data_access_protection */
.org trap_table + (TT_FAST_DATA_ACCESS_PROTECTION+512)*ENTRY_SIZE
SYMBOL(fast_data_access_protection_handler_tl1)
	FAST_DATA_ACCESS_PROTECTION_HANDLER 1

/* TT = 0x7c, TL > 0, cpu_mondo */
.org trap_table + (TT_CPU_MONDO+512)*ENTRY_SIZE
SYMBOL(cpu_mondo_handler_tl1)
	wrpr %g0, %tl
	mov TT_CPU_MONDO, %g2
	clr %g5
	PREEMPTIBLE_HANDLER exc_dispatch

/* TT = 0x80, TL > 0, spill_0_normal handler */
.org trap_table + (TT_SPILL_0_NORMAL+512)*ENTRY_SIZE
SYMBOL(spill_0_normal_tl1)
	SPILL_NORMAL_HANDLER_KERNEL

/* TT = 0x88, TL > 0, spill_2_normal handler */
.org trap_table + (TT_SPILL_2_NORMAL+512)*ENTRY_SIZE
SYMBOL(spill_2_normal_tl1)
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xa0, TL > 0, spill_0_other handler */
.org trap_table + (TT_SPILL_0_OTHER+512)*ENTRY_SIZE
SYMBOL(spill_0_other_tl1)
	SPILL_TO_USPACE_WINDOW_BUFFER

/* TT = 0xc0, TL > 0, fill_0_normal handler */
.org trap_table + (TT_FILL_0_NORMAL+512)*ENTRY_SIZE
SYMBOL(fill_0_normal_tl1)
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

#define NOT(x)	((x) == 0)

/*
 * Perform all the actions of the preemptible trap handler which are common
 * for trapping from kernel and trapping from userspace, including call of the
 * higher level service routine.
 *
 * Important note:
 * 	This macro must be inserted between the "2:" and "4:" labels. The
 *	inserting code must be aware of the usage of all the registers
 *	contained in this macro.
 */
.macro MIDDLE_PART is_syscall
	/* copy higher level routine's address and its argument */
	mov %g1, %l0
.if NOT(\is_syscall)
	mov %g2, %o0
.else
	! store the syscall number on the stack as 7th argument
	stx %g2, [%sp + STACK_BIAS + ISTATE_OFFSET_ARG6]
.endif

	/*
	 * Save TSTATE, TPC and TNPC aside.
	 */
	rdpr %tstate, %g1
	rdpr %tpc, %g2
	rdpr %tnpc, %g3

	stx %g1, [%sp + STACK_BIAS + ISTATE_OFFSET_TSTATE]
	stx %g2, [%sp + STACK_BIAS + ISTATE_OFFSET_TPC]
	stx %g3, [%sp + STACK_BIAS + ISTATE_OFFSET_TNPC]

	/*
	 * Save the Y register.
	 */
	rd %y, %g4
	stx %g4, [%sp + STACK_BIAS + ISTATE_OFFSET_Y]

	/*
	 * Save the faulting page and context.
	 */
	stx %g5, [%sp + STACK_BIAS + ISTATE_OFFSET_TLB_TAG_ACCESS]

	/* switch to TL = 0, explicitly enable FPU */
	wrpr %g0, 0, %tl
	wrpr %g0, 0, %gl
	wrpr %g0, PSTATE_PRIV_BIT | PSTATE_PEF_BIT, %pstate

	/* g1 -> l1, ..., g7 -> l7 */
	SAVE_GLOBALS

.if NOT(\is_syscall)
	/* call higher-level service routine, pass istate as its 2nd parameter */
	call %l0
	add %sp, STACK_BIAS, %o1
.else
	/* Call the higher-level syscall handler. */
	!wrpr %g0, PSTATE_PRIV_BIT | PSTATE_PEF_BIT | PSTATE_IE_BIT, %pstate
	call syscall_handler
	nop
	/* copy the value returned by the syscall */
	mov %o0, %i0
.endif

	/* l1 -> g1, ..., l7 -> g7 */
	RESTORE_GLOBALS

	/* we must prserve the PEF bit */
	rdpr %pstate, %l1

	/* TL := 1, GL := 1 */
	wrpr %g0, PSTATE_PRIV_BIT, %pstate
	wrpr %g0, 1, %tl
	wrpr %g0, 1, %gl

	/* Read TSTATE, TPC and TNPC from saved copy. */
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_TSTATE], %g1
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_TPC], %g2
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_TNPC], %g3

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
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_Y], %g4
	wr %g4, %y

	/* If TSTATE.CWP + 1 == CWP, then we do not have to fix CWP. */
	and %g1, TSTATE_CWP_MASK, %l0
	inc %l0
	and %l0, NWINDOWS - 1, %l0	! %l0 mod NWINDOWS
	rdpr %cwp, %l1
	cmp %l0, %l1
	bz %xcc, 4f			! CWP is ok
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
	stx %i0, [%sp + STACK_BIAS + ISTATE_OFFSET_O0]
	stx %i1, [%sp + STACK_BIAS + ISTATE_OFFSET_O1]
	stx %i2, [%sp + STACK_BIAS + ISTATE_OFFSET_O2]
	stx %i3, [%sp + STACK_BIAS + ISTATE_OFFSET_O3]
	stx %i4, [%sp + STACK_BIAS + ISTATE_OFFSET_O4]
	stx %i5, [%sp + STACK_BIAS + ISTATE_OFFSET_O5]
	stx %i6, [%sp + STACK_BIAS + ISTATE_OFFSET_O6]
	stx %i7, [%sp + STACK_BIAS + ISTATE_OFFSET_O7]
	wrpr %l0, 0, %cwp
	mov %g2, %sp
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O0], %i0
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O1], %i1
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O2], %i2
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O3], %i3
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O4], %i4
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O5], %i5
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O6], %i6
	ldx [%sp + STACK_BIAS + ISTATE_OFFSET_O7], %i7
.endm

/*
 * Preemptible trap handler for handling traps from kernel.
 */
.macro PREEMPTIBLE_HANDLER_KERNEL
	/* prevent unnecessary CLEANWIN exceptions */
	wrpr %g0, NWINDOWS - 1, %cleanwin

	/*
	 * Prevent SAVE instruction from causing a spill exception. If the
	 * CANSAVE register is zero, explicitly spill register window
	 * at CWP + 2.
	 */

	rdpr %cansave, %g3
	brnz %g3, 2f
	nop
	rdpr %otherwin, %g4
	brnz %g4, 1f
	nop

	/* OTHERWIN is zero, we are spilling a kernel window. */
	INLINE_SPILL %g3, %g4
	ba,a %xcc, 2f

1:
	/* OTHERWIN is non-zero, we are spilling a uspace window. */
	INLINE_SPILL_TO_WBUF %g3, %g4, %g7

2:
	/* ask for new register window */
	save %sp, -ISTATE_SIZE, %sp

	MIDDLE_PART 0

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

/*
 * Spills the window at CWP + 2 to the userspace window buffer. This macro
 * is to be used before doing SAVE when the spill trap is undesirable.
 *
 * Parameters:
 * 	tmpreg1		global register to be used for scratching purposes
 * 	tmpreg2		global register to be used for scratching purposes
 * 	tmpreg3		global register to be used for scratching purposes
 */
.macro INLINE_SPILL_TO_WBUF tmpreg1, tmpreg2, tmpreg3
	! CWP := CWP + 2
	rdpr %cwp, \tmpreg2
	add \tmpreg2, 2, \tmpreg1
	and \tmpreg1, NWINDOWS - 1, \tmpreg1		! modulo NWINDOWS
	wrpr \tmpreg1, %cwp

	! spill to userspace window buffer
	SAVE_TO_USPACE_WBUF \tmpreg3, \tmpreg1

	! CWP := CWP - 2
	wrpr \tmpreg2, %cwp

	saved
.endm

/*
 * Preemptible handler for handling traps from userspace.
 */
.macro PREEMPTIBLE_HANDLER_USPACE is_syscall
	/*
	 * One of the ways this handler can be invoked is after a nested MMU trap from
	 * either spill_1_normal or fill_1_normal traps. Both of these traps manipulate
	 * the CWP register. We deal with the situation by simulating the MMU trap
	 * on TL=1 and restart the respective SAVE or RESTORE instruction once the MMU
	 * trap is resolved. However, because we are in the wrong window from the
	 * perspective of the MMU trap, we need to synchronize CWP with CWP from TL=0.
	 */
.if NOT(\is_syscall)
	rdpr %tstate, %g3
	and %g3, TSTATE_CWP_MASK, %g4
	wrpr %g4, 0, %cwp			! resynchronize CWP
.endif

	/* prevent unnecessary CLEANWIN exceptions */
	wrpr %g0, NWINDOWS - 1, %cleanwin

	/*
	 * Prevent SAVE instruction from causing a spill exception. If the
	 * CANSAVE register is zero, explicitly spill register window
	 * at CWP + 2.
	 */
	rdpr %cansave, %g3
	brnz %g3, 2f
	nop
	INLINE_SPILL_TO_WBUF %g3, %g4, %g7

2:
	set SCRATCHPAD_KSTACK, %g4
	ldxa [%g4] ASI_SCRATCHPAD, %g6
	save %g6, -ISTATE_SIZE, %sp

.if \is_syscall
	/* Copy arguments for the syscall to the new window. */
	mov %i0, %o0
	mov %i1, %o1
	mov %i2, %o2
	mov %i3, %o3
	mov %i4, %o4
	mov %i5, %o5
.endif

	mov VA_PRIMARY_CONTEXT_REG, %l0
	stxa %g0, [%l0] ASI_PRIMARY_CONTEXT_REG
	rd %pc, %l0
	flush %l0

	/* Mark the CANRESTORE windows as OTHER windows. */
	rdpr %canrestore, %l0
	wrpr %l0, %otherwin
	wrpr %g0, %canrestore

	/*
	 * Other window spills will go to the userspace window buffer
	 * and normal spills will go to the kernel stack.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(0), %wstate

	MIDDLE_PART \is_syscall

4:
	/*
	 * Spills and fills will be processed by the {spill,fill}_1_normal
	 * handlers.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(1), %wstate

	/*
	 * Set primary context according to secondary context.
	 */
	wr %g0, ASI_SECONDARY_CONTEXT_REG, %asi
	ldxa [VA_SECONDARY_CONTEXT_REG] %asi, %g1
	wr %g0, ASI_PRIMARY_CONTEXT_REG, %asi
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi
	rd %pc, %g1
	flush %g1

	/* Restoring userspace windows: */

	/* Save address of the userspace window buffer to the %g7 register. */
	set SCRATCHPAD_WBUF, %g5
	ldxa [%g5] ASI_SCRATCHPAD, %g7

	rdpr %cwp, %g1
	rdpr %otherwin, %g2

	/*
	 * Skip all OTHERWIN windows and descend to the first window
	 * in the userspace window buffer.
	 */
	sub %g1, %g2, %g3
	dec %g3
	and %g3, NWINDOWS - 1, %g3
	wrpr %g3, 0, %cwp

	/*
	 * CWP is now in the window last saved in the userspace window buffer.
	 * Fill all windows stored in the buffer.
	 */
	clr %g4
5:	andcc %g7, UWB_ALIGNMENT - 1, %g0	! alignment check
	bz %xcc, 6f				! %g7 is UWB_ALIGNMENT-aligned, no more windows to refill
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
	and %g3, NWINDOWS - 1, %g3
	wrpr %g3, 0, %cwp			! switch to the preceeding window

	ba %xcc, 5b
	inc %g4

6:
	/* Save changes of the address of the userspace window buffer. */
	stxa %g7, [%g5] ASI_SCRATCHPAD

	/*
	 * Switch back to the proper current window and adjust
	 * OTHERWIN, CANRESTORE, CANSAVE and CLEANWIN.
	 */
	wrpr %g1, 0, %cwp
	add %g4, %g2, %g2
	cmp %g2, NWINDOWS - 2
	bg %xcc, 8f				! fix the CANRESTORE=NWINDOWS-1 anomaly
	mov NWINDOWS - 2, %g1			! use dealy slot for both cases
	sub %g1, %g2, %g1

	wrpr %g0, 0, %otherwin
	wrpr %g1, 0, %cansave			! NWINDOWS - 2 - CANRESTORE
	wrpr %g2, 0, %canrestore		! OTHERWIN + windows in the buffer
	wrpr %g2, 0, %cleanwin			! avoid information leak

7:
	restore

.if \is_syscall
	done
.else
	retry
.endif

8:
	/*
	 * We got here in order to avoid inconsistency of the window state registers.
	 * If the:
	 *
	 * 	save %g6, -ISTATE_SIZE, %sp
	 *
	 * instruction trapped and spilled a register window into the userspace
	 * window buffer, we have just restored NWINDOWS - 1 register windows.
	 * However, CANRESTORE can be only NWINDOW - 2 at most.
	 *
	 * The solution is to manually switch to (CWP - 1) mod NWINDOWS
	 * and set the window state registers so that:
	 *
	 * 	CANRESTORE 	= NWINDOWS - 2
	 *	CLEANWIN	= NWINDOWS - 2
	 *	CANSAVE 	= 0
	 *	OTHERWIN	= 0
	 *
	 * The RESTORE instruction is therfore to be skipped.
	 */
	wrpr %g0, 0, %otherwin
	wrpr %g0, 0, %cansave
	wrpr %g1, 0, %canrestore
	wrpr %g1, 0, %cleanwin

	rdpr %cwp, %g1
	dec %g1
	and %g1, NWINDOWS - 1, %g1
	wrpr %g1, 0, %cwp			! CWP--

.if \is_syscall
	done
.else
	retry
.endif

.endm

/* Preemptible trap handler for TL=1.
 *
 * This trap handler makes arrangements to make calling of scheduler() from
 * within a trap context possible. It is called from several other trap
 * handlers.
 */
.macro PREEMPTIBLE_HANDLER_TEMPLATE is_syscall
	rdpr %tstate, %g3
	and %g3, TSTATE_PRIV_BIT, %g3
	brz %g3, 100f			! trapping from userspace
	nop

	PREEMPTIBLE_HANDLER_KERNEL
	ba,a %xcc, 101f

	100:
	PREEMPTIBLE_HANDLER_USPACE \is_syscall

	101:
.endm

SYMBOL(preemptible_handler)
	PREEMPTIBLE_HANDLER_TEMPLATE 0

SYMBOL(trap_instruction_handler)
	PREEMPTIBLE_HANDLER_TEMPLATE 1
