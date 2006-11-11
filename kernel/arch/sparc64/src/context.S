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

#include <arch/context_offset.h>

/**
 * Both context_save_arch() and context_restore_arch() are
 * leaf-optimized procedures. This kind of optimization
 * is very important and prevents any implicit window
 * spill/fill/clean traps in these very core kernel
 * functions.
 */
	
.text   

.global context_save_arch
.global context_restore_arch

.macro CONTEXT_STORE r
	stx %sp, [\r + OFFSET_SP]
	stx %o7, [\r + OFFSET_PC]
	stx %i0, [\r + OFFSET_I0]
	stx %i1, [\r + OFFSET_I1]
	stx %i2, [\r + OFFSET_I2]
	stx %i3, [\r + OFFSET_I3]
	stx %i4, [\r + OFFSET_I4]
	stx %i5, [\r + OFFSET_I5]
	stx %fp, [\r + OFFSET_FP]
	stx %i7, [\r + OFFSET_I7]
	stx %l0, [\r + OFFSET_L0]
	stx %l1, [\r + OFFSET_L1]
	stx %l2, [\r + OFFSET_L2]
	stx %l3, [\r + OFFSET_L3]
	stx %l4, [\r + OFFSET_L4]
	stx %l5, [\r + OFFSET_L5]
	stx %l6, [\r + OFFSET_L6]
	stx %l7, [\r + OFFSET_L7]
.endm

.macro CONTEXT_LOAD r
	ldx [\r + OFFSET_SP], %sp
	ldx [\r + OFFSET_PC], %o7
	ldx [\r + OFFSET_I0], %i0
	ldx [\r + OFFSET_I1], %i1
	ldx [\r + OFFSET_I2], %i2
	ldx [\r + OFFSET_I3], %i3
	ldx [\r + OFFSET_I4], %i4
	ldx [\r + OFFSET_I5], %i5
	ldx [\r + OFFSET_FP], %fp
	ldx [\r + OFFSET_I7], %i7
	ldx [\r + OFFSET_L0], %l0
	ldx [\r + OFFSET_L1], %l1
	ldx [\r + OFFSET_L2], %l2
	ldx [\r + OFFSET_L3], %l3
	ldx [\r + OFFSET_L4], %l4
	ldx [\r + OFFSET_L5], %l5
	ldx [\r + OFFSET_L6], %l6
	ldx [\r + OFFSET_L7], %l7
.endm

context_save_arch:
	CONTEXT_STORE %o0
	retl
	mov 1, %o0		! context_save_arch returns 1

context_restore_arch:
	#
	# Flush all active windows.
	# This is essential, because CONTEXT_LOAD overwrites
	# %sp of CWP - 1 with the value written to %fp of CWP.
	# Flushing all active windows mitigates this problem
	# as CWP - 1 becomes the overlap window.
	#
	flushw
	
	CONTEXT_LOAD %o0
	retl
	xor %o0, %o0, %o0	! context_restore_arch returns 0
