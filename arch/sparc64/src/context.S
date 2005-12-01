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
#include <arch/stack.h>

/**
 * Both context_save_arch() and context_restore_arch() are
 * leaf-optimized procedures. Both of them touch %sp and
 * %fp and thus break SCD 2.4 compliance. However, these
 * functions are special enough to be allowed this
 * kind of behavior. Moreover, this kind of optimization
 * is very important and prevents any window spill/
 * fill/clean traps in these very core kernel functions.
 */
	
.text   

.global context_save_arch
.global context_restore_arch

.macro CONTEXT_STORE r
	stx %o1, [\r + OFFSET_O1]
	stx %o2, [\r + OFFSET_O2]
	stx %o3, [\r + OFFSET_O3]
	stx %o4, [\r + OFFSET_O4]
	stx %o5, [\r + OFFSET_O5]
	stx %o7, [\r + OFFSET_PC]
	stx %sp, [\r + OFFSET_SP]
	stx %fp, [\r + OFFSET_FP]
.endm

.macro CONTEXT_LOAD r
	ldx [\r + OFFSET_O1], %o1
	ldx [\r + OFFSET_O2], %o2
	ldx [\r + OFFSET_O3], %o3
	ldx [\r + OFFSET_O4], %o4
	ldx [\r + OFFSET_O5], %o5
	ldx [\r + OFFSET_PC], %o7
	ldx [\r + OFFSET_SP], %sp
	ldx [\r + OFFSET_FP], %fp
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
