#
# Copyright (c) 2005 Jakub Jermar
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
#include <arch/arch.h>
#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/mm/mmu.h>

.text

.register       %g2, #scratch
.register       %g3, #scratch

.macro WRITE_ALTERNATE_REGISTER reg, bit
	rdpr %pstate, %g1				! save PSTATE.PEF
	wrpr %g0, (\bit | PSTATE_PRIV_BIT), %pstate
	mov %o0, \reg
	wrpr %g0, PSTATE_PRIV_BIT, %pstate
	retl
	wrpr %g1, 0, %pstate				! restore PSTATE.PEF
.endm

.macro READ_ALTERNATE_REGISTER reg, bit
	rdpr %pstate, %g1				! save PSTATE.PEF
	wrpr %g0, (\bit | PSTATE_PRIV_BIT), %pstate
	mov \reg, %o0
	wrpr %g0, PSTATE_PRIV_BIT, %pstate
	retl
	wrpr %g1, 0, %pstate				! restore PSTATE.PEF
.endm

FUNCTION_BEGIN(write_to_ag_g6)
	WRITE_ALTERNATE_REGISTER %g6, PSTATE_AG_BIT
FUNCTION_END(write_to_ag_g6)

FUNCTION_BEGIN(write_to_ag_g7)
	WRITE_ALTERNATE_REGISTER %g7, PSTATE_AG_BIT
FUNCTION_END(write_to_ag_g7)

FUNCTION_BEGIN(write_to_ig_g6)
	WRITE_ALTERNATE_REGISTER %g6, PSTATE_IG_BIT
FUNCTION_END(write_to_ig_g6)

FUNCTION_BEGIN(read_from_ag_g6)
	READ_ALTERNATE_REGISTER %g6, PSTATE_AG_BIT
FUNCTION_END(read_from_ag_g6)

FUNCTION_BEGIN(read_from_ag_g7)
	READ_ALTERNATE_REGISTER %g7, PSTATE_AG_BIT
FUNCTION_END(read_from_ag_g7)

/** Switch to userspace.
 *
 * %o0	Userspace entry address.
 * %o1	Userspace stack pointer address.
 * %o2  Userspace address of uarg structure.
 */
FUNCTION_BEGIN(switch_to_userspace)
	save %o1, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	flushw
	wrpr %g0, 0, %cleanwin		! avoid information leak

	mov %i2, %o0			! uarg
	xor %o1, %o1, %o1		! %o1 is defined to hold pcb_ptr
					! set it to 0

	clr %i2
	clr %i3
	clr %i4
	clr %i5
	clr %i6

	wrpr %g0, 1, %tl		! enforce mapping via nucleus

	rdpr %cwp, %g1
	wrpr %g1, TSTATE_IE_BIT, %tstate
	wrpr %i0, 0, %tnpc

	/*
	 * Set primary context according to secondary context.
	 * Secondary context has been already installed by
	 * higher-level functions.
	 */
	wr %g0, ASI_DMMU, %asi
	ldxa [VA_SECONDARY_CONTEXT_REG] %asi, %g1
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi
	flush %i7

	/*
	 * Spills and fills will be handled by the userspace handlers.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(1), %wstate

	done				! jump to userspace
FUNCTION_END(switch_to_userspace)

