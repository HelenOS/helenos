/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <arch/trap/interrupt.h>
#include <arch/trap/exception.h>
#include <arch/trap/mmu.h>
#include <arch/sparc64.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <stdbool.h>
#include <debug.h>
#include <arch/asm.h>
#include <barrier.h>
#include <arch/drivers/tick.h>
#include <arch.h>
#include <mm/tlb.h>
#include <config.h>
#include <synch/spinlock.h>

void exc_arch_init(void)
{
	exc_register(TT_INSTRUCTION_ACCESS_EXCEPTION,
	    "instruction_access_exception", false,
	    instruction_access_exception);
	exc_register(TT_INSTRUCTION_ACCESS_ERROR,
	    "instruction_access_error", false,
	    instruction_access_error);

#ifdef SUN4V
	exc_register(TT_IAE_UNAUTH_ACCESS,
	    "iae_unauth_access", false,
	    instruction_access_exception);
	exc_register(TT_IAE_NFO_PAGE,
	    "iae_nfo_page", false,
	    instruction_access_exception);
#endif

	exc_register(TT_ILLEGAL_INSTRUCTION,
	    "illegal_instruction", false,
	    illegal_instruction);
	exc_register(TT_PRIVILEGED_OPCODE,
	    "privileged_opcode", false,
	    privileged_opcode);
	exc_register(TT_UNIMPLEMENTED_LDD,
	    "unimplemented_LDD", false,
	    unimplemented_LDD);
	exc_register(TT_UNIMPLEMENTED_STD,
	    "unimplemented_STD", false,
	    unimplemented_STD);

#ifdef SUN4V
	exc_register(TT_DAE_INVALID_ASI,
	    "dae_invalid_asi", false,
	    data_access_exception);
	exc_register(TT_DAE_PRIVILEGE_VIOLATION,
	    "dae_privilege_violation", false,
	    data_access_exception);
	exc_register(TT_DAE_NC_PAGE,
	    "dae_nc_page", false,
	    data_access_exception);
	exc_register(TT_DAE_NC_PAGE,
	    "dae_nc_page", false,
	    data_access_exception);
	exc_register(TT_DAE_NFO_PAGE,
	    "dae_nfo_page", false,
	    data_access_exception);
#endif

	exc_register(TT_FP_DISABLED,
	    "fp_disabled", true,
	    fp_disabled);
	exc_register(TT_FP_EXCEPTION_IEEE_754,
	    "fp_exception_ieee_754", false,
	    fp_exception_ieee_754);
	exc_register(TT_FP_EXCEPTION_OTHER,
	    "fp_exception_other", false,
	    fp_exception_other);
	exc_register(TT_TAG_OVERFLOW,
	    "tag_overflow", false,
	    tag_overflow);
	exc_register(TT_DIVISION_BY_ZERO,
	    "division_by_zero", false,
	    division_by_zero);
	exc_register(TT_DATA_ACCESS_EXCEPTION,
	    "data_access_exception", false,
	    data_access_exception);
	exc_register(TT_DATA_ACCESS_ERROR,
	    "data_access_error", false,
	    data_access_error);
	exc_register(TT_MEM_ADDRESS_NOT_ALIGNED,
	    "mem_address_not_aligned", false,
	    mem_address_not_aligned);
	exc_register(TT_LDDF_MEM_ADDRESS_NOT_ALIGNED,
	    "LDDF_mem_address_not_aligned", false,
	    LDDF_mem_address_not_aligned);
	exc_register(TT_STDF_MEM_ADDRESS_NOT_ALIGNED,
	    "STDF_mem_address_not_aligned", false,
	    STDF_mem_address_not_aligned);
	exc_register(TT_PRIVILEGED_ACTION,
	    "privileged_action", false,
	    privileged_action);
	exc_register(TT_LDQF_MEM_ADDRESS_NOT_ALIGNED,
	    "LDQF_mem_address_not_aligned", false,
	    LDQF_mem_address_not_aligned);
	exc_register(TT_STQF_MEM_ADDRESS_NOT_ALIGNED,
	    "STQF_mem_address_not_aligned", false,
	    STQF_mem_address_not_aligned);

	exc_register(TT_INTERRUPT_LEVEL_14,
	    "interrupt_level_14", true,
	    tick_interrupt);

#ifdef SUN4U
	exc_register(TT_INTERRUPT_VECTOR_TRAP,
	    "interrupt_vector_trap", true,
	    interrupt);
#endif

	exc_register(TT_FAST_INSTRUCTION_ACCESS_MMU_MISS,
	    "fast_instruction_access_mmu_miss", true,
	    fast_instruction_access_mmu_miss);
	exc_register(TT_FAST_DATA_ACCESS_MMU_MISS,
	    "fast_data_access_mmu_miss", true,
	    fast_data_access_mmu_miss);
	exc_register(TT_FAST_DATA_ACCESS_PROTECTION,
	    "fast_data_access_protection", true,
	    fast_data_access_protection);

#ifdef SUN4V
	exc_register(TT_CPU_MONDO,
	    "cpu_mondo", true,
	    cpu_mondo);
#endif

}

/** @}
 */
