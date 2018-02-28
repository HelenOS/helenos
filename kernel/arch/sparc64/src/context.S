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
#include <arch/context_struct.h>
#include <arch/arch.h>
#include <arch/regdef.h>

.text

/*
 * context_save_arch() is required not to create its own stack frame. See the
 * generic context.h for explanation.
 */
FUNCTION_BEGIN(context_save_arch)
	#
	# Force all our active register windows to memory so that we can find
	# them there even if e.g. the thread is migrated to another processor.
	#
	flushw

	stx %sp, [%o0 + CONTEXT_OFFSET_SP]
	stx %o7, [%o0 + CONTEXT_OFFSET_PC]
	stx %i0, [%o0 + CONTEXT_OFFSET_I0]
	stx %i1, [%o0 + CONTEXT_OFFSET_I1]
	stx %i2, [%o0 + CONTEXT_OFFSET_I2]
	stx %i3, [%o0 + CONTEXT_OFFSET_I3]
	stx %i4, [%o0 + CONTEXT_OFFSET_I4]
	stx %i5, [%o0 + CONTEXT_OFFSET_I5]
	stx %fp, [%o0 + CONTEXT_OFFSET_FP]
	stx %i7, [%o0 + CONTEXT_OFFSET_I7]
	stx %l0, [%o0 + CONTEXT_OFFSET_L0]
	stx %l1, [%o0 + CONTEXT_OFFSET_L1]
	stx %l2, [%o0 + CONTEXT_OFFSET_L2]
	stx %l3, [%o0 + CONTEXT_OFFSET_L3]
	stx %l4, [%o0 + CONTEXT_OFFSET_L4]
	stx %l5, [%o0 + CONTEXT_OFFSET_L5]
	stx %l6, [%o0 + CONTEXT_OFFSET_L6]
	stx %l7, [%o0 + CONTEXT_OFFSET_L7]
	retl
	mov 1, %o0		! context_save_arch returns 1
FUNCTION_END(context_save_arch)

FUNCTION_BEGIN(context_restore_arch)
	#
	# Forget all previous windows, they are not going to be needed again.
	# Enforce a window fill on the next RESTORE instruction by setting
	# CANRESTORE to zero and other window configuration registers
	# accordingly. Note that the same can be achieved by executing the
	# FLUSHW instruction, but since we don't need to remember the previous
	# windows, we do the former and save thus some unnecessary window
	# spills.
	#
	rdpr %pstate, %l0
	andn %l0, PSTATE_IE_BIT, %l1
	wrpr %l1, %pstate
	wrpr %g0, 0, %canrestore
	wrpr %g0, 0, %otherwin
	wrpr %g0, NWINDOWS - 2, %cansave
	wrpr %l0, %pstate

	ldx [%o0 + CONTEXT_OFFSET_SP], %sp
	ldx [%o0 + CONTEXT_OFFSET_PC], %o7
	ldx [%o0 + CONTEXT_OFFSET_I0], %i0
	ldx [%o0 + CONTEXT_OFFSET_I1], %i1
	ldx [%o0 + CONTEXT_OFFSET_I2], %i2
	ldx [%o0 + CONTEXT_OFFSET_I3], %i3
	ldx [%o0 + CONTEXT_OFFSET_I4], %i4
	ldx [%o0 + CONTEXT_OFFSET_I5], %i5
	ldx [%o0 + CONTEXT_OFFSET_FP], %fp
	ldx [%o0 + CONTEXT_OFFSET_I7], %i7
	ldx [%o0 + CONTEXT_OFFSET_L0], %l0
	ldx [%o0 + CONTEXT_OFFSET_L1], %l1
	ldx [%o0 + CONTEXT_OFFSET_L2], %l2
	ldx [%o0 + CONTEXT_OFFSET_L3], %l3
	ldx [%o0 + CONTEXT_OFFSET_L4], %l4
	ldx [%o0 + CONTEXT_OFFSET_L5], %l5
	ldx [%o0 + CONTEXT_OFFSET_L6], %l6
	ldx [%o0 + CONTEXT_OFFSET_L7], %l7
	retl
	xor %o0, %o0, %o0	! context_restore_arch returns 0
FUNCTION_END(context_restore_arch)
