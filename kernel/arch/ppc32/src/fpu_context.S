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

#include <arch/asm/regname.h>
#include <arch/context_offset.h>

.text

.global fpu_context_save
.global fpu_context_restore
.global fpu_init
.global fpu_enable
.global fpu_disable

.macro FPU_CONTEXT_STORE r
	stfd fr14, OFFSET_FR14(\r)
	stfd fr15, OFFSET_FR15(\r)
	stfd fr16, OFFSET_FR16(\r)
	stfd fr17, OFFSET_FR17(\r)
	stfd fr18, OFFSET_FR18(\r)
	stfd fr19, OFFSET_FR19(\r)
	stfd fr20, OFFSET_FR20(\r)
	stfd fr21, OFFSET_FR21(\r)
	stfd fr22, OFFSET_FR22(\r)
	stfd fr23, OFFSET_FR23(\r)
	stfd fr24, OFFSET_FR24(\r)
	stfd fr25, OFFSET_FR25(\r)
	stfd fr26, OFFSET_FR26(\r)
	stfd fr27, OFFSET_FR27(\r)
	stfd fr28, OFFSET_FR28(\r)
	stfd fr29, OFFSET_FR29(\r)
	stfd fr30, OFFSET_FR30(\r)
	stfd fr31, OFFSET_FR31(\r)
.endm

.macro FPU_CONTEXT_LOAD r
	lfd fr14, OFFSET_FR14(\r)
	lfd fr15, OFFSET_FR15(\r)
	lfd fr16, OFFSET_FR16(\r)
	lfd fr17, OFFSET_FR17(\r)
	lfd fr18, OFFSET_FR18(\r)
	lfd fr19, OFFSET_FR19(\r)
	lfd fr20, OFFSET_FR20(\r)
	lfd fr21, OFFSET_FR21(\r)
	lfd fr22, OFFSET_FR22(\r)
	lfd fr23, OFFSET_FR23(\r)
	lfd fr24, OFFSET_FR24(\r)
	lfd fr25, OFFSET_FR25(\r)
	lfd fr26, OFFSET_FR26(\r)
	lfd fr27, OFFSET_FR27(\r)
	lfd fr28, OFFSET_FR28(\r)
	lfd fr29, OFFSET_FR29(\r)
	lfd fr30, OFFSET_FR30(\r)
	lfd fr31, OFFSET_FR31(\r)
.endm

fpu_context_save:
//	FPU_CONTEXT_STORE r3
//	
//	mffs fr0
//	stfd fr0, OFFSET_FPSCR(r3)
	
	blr

fpu_context_restore:
//	FPU_CONTEXT_LOAD r3
//	
//	lfd fr0, OFFSET_FPSCR(r3)
//	mtfsf 7, fr0
	
	blr

fpu_init:
	blr

fpu_enable:
	blr

fpu_disable:
	blr
