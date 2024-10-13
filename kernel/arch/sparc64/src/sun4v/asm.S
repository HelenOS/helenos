#
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

#include <abi/asmtool.h>
#include <arch/mm/mmu.h>
#include <arch/regdef.h>
#include <arch/stack.h>

.text

/** Switch to userspace.
 *
 * %o0	Userspace entry address.
 * %o1	Userspace stack pointer address.
 * %o2  Userspace address of uarg structure.
 */
FUNCTION_BEGIN(switch_to_userspace)
	save %o1, 0, %sp
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
	wr %g0, ASI_SECONDARY_CONTEXT_REG, %asi
	ldxa [VA_SECONDARY_CONTEXT_REG] %asi, %g1
	wr %g0, ASI_PRIMARY_CONTEXT_REG, %asi
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi
	flush %i7

	/*
	 * Spills and fills will be handled by the userspace handlers.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(1), %wstate
	done				! jump to userspace
FUNCTION_END(switch_to_userspace)
