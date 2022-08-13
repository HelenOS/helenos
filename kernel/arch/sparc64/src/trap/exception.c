/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/** @file
 *
 */

#include <arch/trap/exception.h>
#include <arch/mm/tlb.h>
#include <arch/interrupt.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <debug.h>
#include <stdio.h>
#include <symtab.h>

void istate_decode(istate_t *istate)
{
	const char *tpcs = symtab_fmt_name_lookup(istate->tpc);
	const char *tnpcs = symtab_fmt_name_lookup(istate->tnpc);

	printf("TSTATE=%#" PRIx64 "\n", istate->tstate);
	printf("TPC=%#" PRIx64 " (%s)\n", istate->tpc, tpcs);
	printf("TNPC=%#" PRIx64 " (%s)\n", istate->tnpc, tnpcs);
}

/** Handle instruction_access_exception. (0x8) */
void instruction_access_exception(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle instruction_access_error. (0xa) */
void instruction_access_error(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle illegal_instruction. (0x10) */
void illegal_instruction(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle privileged_opcode. (0x11) */
void privileged_opcode(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle unimplemented_LDD. (0x12) */
void unimplemented_LDD(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle unimplemented_STD. (0x13) */
void unimplemented_STD(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle fp_disabled. (0x20) */
void fp_disabled(unsigned int n, istate_t *istate)
{
	fprs_reg_t fprs;

	fprs.value = fprs_read();
	if (!fprs.fef) {
		fprs.fef = true;
		fprs_write(fprs.value);
		return;
	}

#ifdef CONFIG_FPU_LAZY
	scheduler_fpu_lazy_request();
#else
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
#endif
}

/** Handle fp_exception_ieee_754. (0x21) */
void fp_exception_ieee_754(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle fp_exception_other. (0x22) */
void fp_exception_other(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle tag_overflow. (0x23) */
void tag_overflow(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle division_by_zero. (0x28) */
void division_by_zero(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle data_access_exception. (0x30) */
void data_access_exception(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle data_access_error. (0x32) */
void data_access_error(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle mem_address_not_aligned. (0x34) */
void mem_address_not_aligned(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle LDDF_mem_address_not_aligned. (0x35) */
void LDDF_mem_address_not_aligned(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle STDF_mem_address_not_aligned. (0x36) */
void STDF_mem_address_not_aligned(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle privileged_action. (0x37) */
void privileged_action(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle LDQF_mem_address_not_aligned. (0x38) */
void LDQF_mem_address_not_aligned(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle STQF_mem_address_not_aligned. (0x39) */
void STQF_mem_address_not_aligned(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** @}
 */
