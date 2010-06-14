#
# Copyright (c) 2006 Martin Decky
# Copyright (c) 2006 Jakub Jermar 
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

#include <arch/arch.h>

.text

.global halt
.global memcpy
.global jump_to_kernel

halt:
	br halt

memcpy:
	alloc loc0 = ar.pfs, 3, 1, 0, 0

	adds r14 = 7, in1
	mov r2 = ar.lc
	mov r8 = in0 ;;
	and r14 = -8, r14 ;;
	cmp.ne p6, p7 = r14, in1
(p7)	br.cond.dpnt 3f ;;
0:
	cmp.ne p6, p7 = 0, in2
(p7)	br.cond.dpnt 2f ;;
(p6)	adds r14 = -1, in2
(p6)	mov r16 = r0
(p6)	mov r17 = r0 ;;
(p6)	mov ar.lc = r14
1:
	add r14 = r16, in1 
	add r15 = r16, in0
	adds r17 = 1, r17 ;;
	ld1 r14 = [r14]
	mov r16 = r17 ;;
	st1 [r15] = r14
	br.cloop.sptk.few 1b ;;
2:
	mov ar.lc = r2
	mov ar.pfs = loc0
	br.ret.sptk.many rp
3:
	adds r14 = 7, in0 ;;
	and r14 = -8, r14 ;;
	cmp.eq p6, p7 = r14, in0
(p7)	br.cond.dptk 0b
	shr.u r18 = in2, 3 ;;
	cmp.ne p6, p7 = 0, r18
(p7)	br.cond.dpnt 5f	;;
(p6)	adds r14 = -1, r18
(p6)	mov r16 = r0
(p6)	mov r17 = r0 ;;
(p6)	mov ar.lc = r14
4:
	shladd r14 = r16, 3, r0
	adds r16 = 1, r17 ;;
	add r15 = in1, r14
	add r14 = in0, r14
	mov r17 = r16 ;;
	ld8 r15 = [r15] ;;
	st8 [r14] = r15
	br.cloop.sptk.few 4b
5:
	and r15 = 7, in2
	shladd r14 = r18, 3, r0
	mov r16 = r0
	mov r18 = r0 ;;
	cmp.eq p6, p7 = 0, r15
	add in0 = r14, in0
	adds r15 = -1, r15
	add r17 = r14, in1 
(p6)	br.cond.dpnt 2b ;;
	mov ar.lc = r15
6:
	add r14 = r16, r17
	add r15 = r16, in0
	adds r16 = 1, r18 ;;
	ld1 r14 = [r14]
	mov r18 = r16 ;;
	st1 [r15] = r14
	br.cloop.sptk.few 6b ;;
	mov ar.lc = r2
	mov ar.pfs = loc0
	br.ret.sptk.many rp

jump_to_kernel:
	alloc loc0 = ar.pfs, 1, 1, 0, 0
	mov r1 = in0 ;;			# Pass bootinfo address
	movl r8 = KERNEL_ADDRESS;;
	mov b1 = r8 ;;
	br.call.sptk.many b0 = b1;;
