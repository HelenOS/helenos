#
# Copyright (c) 2006 Martin Decky
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
#include <arch/asm/regname.h>
#include <arch/fpu_context_struct.h>
#include <arch/msr.h>

.text

.macro FPU_CONTEXT_STORE r
	stfd fr0, FPU_CONTEXT_OFFSET_FR0(\r)
	stfd fr1, FPU_CONTEXT_OFFSET_FR1(\r)
	stfd fr2, FPU_CONTEXT_OFFSET_FR2(\r)
	stfd fr3, FPU_CONTEXT_OFFSET_FR3(\r)
	stfd fr4, FPU_CONTEXT_OFFSET_FR4(\r)
	stfd fr5, FPU_CONTEXT_OFFSET_FR5(\r)
	stfd fr6, FPU_CONTEXT_OFFSET_FR6(\r)
	stfd fr7, FPU_CONTEXT_OFFSET_FR7(\r)
	stfd fr8, FPU_CONTEXT_OFFSET_FR8(\r)
	stfd fr9, FPU_CONTEXT_OFFSET_FR9(\r)
	stfd fr10, FPU_CONTEXT_OFFSET_FR10(\r)
	stfd fr11, FPU_CONTEXT_OFFSET_FR11(\r)
	stfd fr12, FPU_CONTEXT_OFFSET_FR12(\r)
	stfd fr13, FPU_CONTEXT_OFFSET_FR13(\r)
	stfd fr14, FPU_CONTEXT_OFFSET_FR14(\r)
	stfd fr15, FPU_CONTEXT_OFFSET_FR15(\r)
	stfd fr16, FPU_CONTEXT_OFFSET_FR16(\r)
	stfd fr17, FPU_CONTEXT_OFFSET_FR17(\r)
	stfd fr18, FPU_CONTEXT_OFFSET_FR18(\r)
	stfd fr19, FPU_CONTEXT_OFFSET_FR19(\r)
	stfd fr20, FPU_CONTEXT_OFFSET_FR20(\r)
	stfd fr21, FPU_CONTEXT_OFFSET_FR21(\r)
	stfd fr22, FPU_CONTEXT_OFFSET_FR22(\r)
	stfd fr23, FPU_CONTEXT_OFFSET_FR23(\r)
	stfd fr24, FPU_CONTEXT_OFFSET_FR24(\r)
	stfd fr25, FPU_CONTEXT_OFFSET_FR25(\r)
	stfd fr26, FPU_CONTEXT_OFFSET_FR26(\r)
	stfd fr27, FPU_CONTEXT_OFFSET_FR27(\r)
	stfd fr28, FPU_CONTEXT_OFFSET_FR28(\r)
	stfd fr29, FPU_CONTEXT_OFFSET_FR29(\r)
	stfd fr30, FPU_CONTEXT_OFFSET_FR30(\r)
	stfd fr31, FPU_CONTEXT_OFFSET_FR31(\r)
.endm

.macro FPU_CONTEXT_LOAD r
	lfd fr0, FPU_CONTEXT_OFFSET_FR0(\r)
	lfd fr1, FPU_CONTEXT_OFFSET_FR1(\r)
	lfd fr2, FPU_CONTEXT_OFFSET_FR2(\r)
	lfd fr3, FPU_CONTEXT_OFFSET_FR3(\r)
	lfd fr4, FPU_CONTEXT_OFFSET_FR4(\r)
	lfd fr5, FPU_CONTEXT_OFFSET_FR5(\r)
	lfd fr6, FPU_CONTEXT_OFFSET_FR6(\r)
	lfd fr7, FPU_CONTEXT_OFFSET_FR7(\r)
	lfd fr8, FPU_CONTEXT_OFFSET_FR8(\r)
	lfd fr9, FPU_CONTEXT_OFFSET_FR9(\r)
	lfd fr10, FPU_CONTEXT_OFFSET_FR10(\r)
	lfd fr11, FPU_CONTEXT_OFFSET_FR11(\r)
	lfd fr12, FPU_CONTEXT_OFFSET_FR12(\r)
	lfd fr13, FPU_CONTEXT_OFFSET_FR13(\r)
	lfd fr14, FPU_CONTEXT_OFFSET_FR14(\r)
	lfd fr15, FPU_CONTEXT_OFFSET_FR15(\r)
	lfd fr16, FPU_CONTEXT_OFFSET_FR16(\r)
	lfd fr17, FPU_CONTEXT_OFFSET_FR17(\r)
	lfd fr18, FPU_CONTEXT_OFFSET_FR18(\r)
	lfd fr19, FPU_CONTEXT_OFFSET_FR19(\r)
	lfd fr20, FPU_CONTEXT_OFFSET_FR20(\r)
	lfd fr21, FPU_CONTEXT_OFFSET_FR21(\r)
	lfd fr22, FPU_CONTEXT_OFFSET_FR22(\r)
	lfd fr23, FPU_CONTEXT_OFFSET_FR23(\r)
	lfd fr24, FPU_CONTEXT_OFFSET_FR24(\r)
	lfd fr25, FPU_CONTEXT_OFFSET_FR25(\r)
	lfd fr26, FPU_CONTEXT_OFFSET_FR26(\r)
	lfd fr27, FPU_CONTEXT_OFFSET_FR27(\r)
	lfd fr28, FPU_CONTEXT_OFFSET_FR28(\r)
	lfd fr29, FPU_CONTEXT_OFFSET_FR29(\r)
	lfd fr30, FPU_CONTEXT_OFFSET_FR30(\r)
	lfd fr31, FPU_CONTEXT_OFFSET_FR31(\r)
.endm

FUNCTION_BEGIN(fpu_context_save)
	FPU_CONTEXT_STORE r3

	mffs fr0
	stfd fr0, FPU_CONTEXT_OFFSET_FPSCR(r3)

	blr
FUNCTION_END(fpu_context_save)

FUNCTION_BEGIN(fpu_context_restore)
	lfd fr0, FPU_CONTEXT_OFFSET_FPSCR(r3)
	mtfsf 0xff, fr0

	FPU_CONTEXT_LOAD r3

	blr
FUNCTION_END(fpu_context_restore)

FUNCTION_BEGIN(fpu_init)
	mfmsr r0
	ori r0, r0, MSR_FP

	# Disable FPU exceptions
	li r3, MSR_FE0 | MSR_FE1
	andc r0, r0, r3

	mtmsr r0
	isync

	blr
FUNCTION_END(fpu_init)

FUNCTION_BEGIN(fpu_enable)
	mfmsr r0
	ori r0, r0, MSR_FP
	mtmsr r0
	isync
	blr
FUNCTION_END(fpu_enable)

FUNCTION_BEGIN(fpu_disable)
	mfmsr r0
	li r3, MSR_FP
	andc r0, r0, r3
	mtmsr r0
	isync
	blr
FUNCTION_END(fpu_disable)
