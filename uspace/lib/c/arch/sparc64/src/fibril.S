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
#include <libarch/fibril_context.h>

.text

FUNCTION_BEGIN(__context_save)
	#
	# We rely on the kernel to flush our active register windows to memory
	# should a thread switch occur.
	#
	stx %sp, [%o0 + __CONTEXT_OFFSET_SP]
	stx %o7, [%o0 + __CONTEXT_OFFSET_PC]
	stx %i0, [%o0 + __CONTEXT_OFFSET_I0]
	stx %i1, [%o0 + __CONTEXT_OFFSET_I1]
	stx %i2, [%o0 + __CONTEXT_OFFSET_I2]
	stx %i3, [%o0 + __CONTEXT_OFFSET_I3]
	stx %i4, [%o0 + __CONTEXT_OFFSET_I4]
	stx %i5, [%o0 + __CONTEXT_OFFSET_I5]
	stx %fp, [%o0 + __CONTEXT_OFFSET_FP]
	stx %i7, [%o0 + __CONTEXT_OFFSET_I7]
	stx %l0, [%o0 + __CONTEXT_OFFSET_L0]
	stx %l1, [%o0 + __CONTEXT_OFFSET_L1]
	stx %l2, [%o0 + __CONTEXT_OFFSET_L2]
	stx %l3, [%o0 + __CONTEXT_OFFSET_L3]
	stx %l4, [%o0 + __CONTEXT_OFFSET_L4]
	stx %l5, [%o0 + __CONTEXT_OFFSET_L5]
	stx %l6, [%o0 + __CONTEXT_OFFSET_L6]
	stx %l7, [%o0 + __CONTEXT_OFFSET_L7]
	stx %g7, [%o0 + __CONTEXT_OFFSET_TP]
	retl
	mov 0, %o0		! __context_save returns 0
FUNCTION_END(__context_save)

FUNCTION_BEGIN(__context_restore)
	#
	# Flush all active windows.
	# This is essential, because CONTEXT_RESTORE_ARCH_CORE overwrites %sp of
	# CWP - 1 with the value written to %fp of CWP.  Flushing all active
	# windows mitigates this problem as CWP - 1 becomes the overlap window.
	#
	flushw

	ldx [%o0 + __CONTEXT_OFFSET_SP], %sp
	ldx [%o0 + __CONTEXT_OFFSET_PC], %o7
	ldx [%o0 + __CONTEXT_OFFSET_I0], %i0
	ldx [%o0 + __CONTEXT_OFFSET_I1], %i1
	ldx [%o0 + __CONTEXT_OFFSET_I2], %i2
	ldx [%o0 + __CONTEXT_OFFSET_I3], %i3
	ldx [%o0 + __CONTEXT_OFFSET_I4], %i4
	ldx [%o0 + __CONTEXT_OFFSET_I5], %i5
	ldx [%o0 + __CONTEXT_OFFSET_FP], %fp
	ldx [%o0 + __CONTEXT_OFFSET_I7], %i7
	ldx [%o0 + __CONTEXT_OFFSET_L0], %l0
	ldx [%o0 + __CONTEXT_OFFSET_L1], %l1
	ldx [%o0 + __CONTEXT_OFFSET_L2], %l2
	ldx [%o0 + __CONTEXT_OFFSET_L3], %l3
	ldx [%o0 + __CONTEXT_OFFSET_L4], %l4
	ldx [%o0 + __CONTEXT_OFFSET_L5], %l5
	ldx [%o0 + __CONTEXT_OFFSET_L6], %l6
	ldx [%o0 + __CONTEXT_OFFSET_L7], %l7
	ldx [%o0 + __CONTEXT_OFFSET_TP], %g7
	retl
	mov %o1, %o0	! __context_restore returns second argument
FUNCTION_END(__context_restore)
