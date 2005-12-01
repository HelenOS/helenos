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
	
.text   

.global context_save_arch
.global context_restore_arch

.macro CONTEXT_STORE r
	stx %i1, [\r + OFFSET_I1]
	stx %i2, [\r + OFFSET_I2]
	stx %i3, [\r + OFFSET_I3]
	stx %i4, [\r + OFFSET_I4]
	stx %i5, [\r + OFFSET_I5]
	stx %fp, [\r + OFFSET_FP]
	stx %i7, [\r + OFFSET_PC]
	stx %sp, [\r + OFFSET_SP]
.endm

.macro CONTEXT_LOAD r
	ldx [\r + OFFSET_I1], %i1
	ldx [\r + OFFSET_I2], %i2
	ldx [\r + OFFSET_I3], %i3
	ldx [\r + OFFSET_I4], %i4
	ldx [\r + OFFSET_I5], %i5
	ldx [\r + OFFSET_FP], %fp
	ldx [\r + OFFSET_PC], %i7
	ldx [\r + OFFSET_SP], %sp
.endm

	
context_save_arch:
	save %sp, -STACK_WINDOW_SAVE_AREA_SIZE, %sp
	CONTEXT_STORE %i0

	# context_save returns 1
	mov 1, %i0
	ret
	restore %sp, STACK_WINDOW_SAVE_AREA_SIZE, %sp

context_restore_arch:
	save %sp, -STACK_WINDOW_SAVE_AREA_SIZE, %sp
	CONTEXT_LOAD %i0

	# context_restore returns 0
	xor %i0, %i0, %i0
	ret
	restore %sp, STACK_WINDOW_SAVE_AREA_SIZE, %sp
