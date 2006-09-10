/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <arch/trap/exception.h>
#include <arch/interrupt.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <debug.h>
#include <typedefs.h>
#include <symtab.h>
#include <print.h>

void dump_istate(istate_t *istate)
{
	printf("TSTATE=%#llx\n", istate->tstate);
	printf("TPC=%#llx (%s)\n", istate->tpc, get_symtab_entry(istate->tpc));
	printf("TNPC=%#llx (%s)\n", istate->tnpc, get_symtab_entry(istate->tnpc));
}

/** Handle instruction_access_exception. (0x8) */
void instruction_access_exception(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle instruction_access_error. (0xa) */
void instruction_access_error(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle illegal_instruction. (0x10) */
void illegal_instruction(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle privileged_opcode. (0x11) */
void privileged_opcode(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle division_by_zero. (0x28) */
void division_by_zero(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle data_access_exception. (0x30) */
void data_access_exception(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle data_access_error. (0x32) */
void data_access_error(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle mem_address_not_aligned. (0x34) */
void mem_address_not_aligned(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** Handle privileged_action. (0x37) */
void privileged_action(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "%s\n", __FUNCTION__);
	dump_istate(istate);
	panic("%s\n", __FUNCTION__);
}

/** @}
 */
