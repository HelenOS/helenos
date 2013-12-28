/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2013 Jakub Klama
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup sparc64interrupt
 * @{
 */
/** @file
 *
 */

#include <arch.h>
#include <typedefs.h>
#include <arch/istate.h>
#include <arch/exception.h>
#include <arch/regwin.h>
#include <arch/machine_func.h>
#include <syscall/syscall.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/as.h>
#include <memstr.h>
#include <debug.h>
#include <print.h>
#include <symtab.h>

/** Handle instruction_access_exception. (0x1) */
void instruction_access_exception(int n, istate_t *istate)
{
	page_fault(n, istate);
}

/** Handle instruction_access_error. (0x21) */
void instruction_access_error(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle illegal_instruction. (0x2) */
void illegal_instruction(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle privileged_instruction. (0x3) */
void privileged_instruction(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle fp_disabled. (0x3) */
void fp_disabled(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle fp_exception. (0x08) */
void fp_exception(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle tag_overflow. (0x0a) */
void tag_overflow(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle division_by_zero. (0x2a) */
void division_by_zero(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle data_access_exception. (0x9) */
void data_access_exception(int n, istate_t *istate)
{
	page_fault(n, istate);
}

/** Handle data_access_error. (0x29) */
void data_access_error(int n, istate_t *istate)
{
	page_fault(n, istate);
}

/** Handle data_store_error. (0x29) */
void data_store_error(int n, istate_t *istate)
{
	page_fault(n, istate);
}

/** Handle data_access_error. (0x2c) */
void data_access_mmu_miss(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

/** Handle mem_address_not_aligned. (0x7) */
void mem_address_not_aligned(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s.", __func__);
	panic_badtrap(istate, n, "%s.", __func__);
}

sysarg_t syscall(sysarg_t a1, sysarg_t a2, sysarg_t a3, sysarg_t a4,
    sysarg_t a5, sysarg_t a6, sysarg_t id)
{
	return syscall_handler(a1, a2, a3, a4, a5, a6, id);
}

void irq_exception(unsigned int nr, istate_t *istate)
{
	machine_irq_exception(nr, istate);
}

void preemptible_save_uspace(uintptr_t sp, istate_t *istate)
{
	as_page_fault(sp, PF_ACCESS_WRITE, istate);
}

void preemptible_restore_uspace(uintptr_t sp, istate_t *istate)
{
	as_page_fault(sp, PF_ACCESS_WRITE, istate);
}

/** @}
 */
