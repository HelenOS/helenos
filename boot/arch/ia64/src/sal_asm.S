#
# Copyright (c) 2011 Jakub Jermar
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

.explicit

#define STACK_SCRATCH_AREA	16
#define STACK_IN8		(0 + STACK_SCRATCH_AREA)
#define STACK_IN9		(8 + STACK_SCRATCH_AREA)
#define STACK_IN10		(16 + STACK_SCRATCH_AREA)

#
# Assume the following PSR bits are already 0, as expected by SAL:
#
#  be, cpl, is, da, dd, ss, ri, ed, ia
#
# Assume PSR.bn is 1, as expected by SAL.
#
FUNCTION_BEGIN(sal_call)
	alloc loc0 = ar.pfs, 8, 8, 8, 0

	adds sp = -STACK_SCRATCH_AREA, sp

	mov loc1 = gp
	mov loc2 = rp

	addl loc3 = @gprel(sal_proc), gp
	addl loc4 = @gprel(sal_proc_gp), gp

	mov out0 = in0
	mov out1 = in1
	mov out2 = in2
	mov out3 = in3
	mov out4 = in4
	mov out5 = in5
	mov out6 = in6
	mov out7 = in7 ;;

	ld8 loc3 = [loc3]
	ld8 gp = [loc4]

	adds r14 = STACK_IN8 + STACK_SCRATCH_AREA, sp
	adds r15 = STACK_IN9 + STACK_SCRATCH_AREA, sp
	adds r16 = STACK_IN10 + STACK_SCRATCH_AREA, sp ;;

	ld8 loc5 = [r14]
	ld8 loc6 = [r15]
	ld8 loc7 = [r16]

	mov b6 = loc3 ;;
	br.call.sptk.many rp = b6

	cmp.ne p7,p0 = 0, loc5
	cmp.ne p8,p0 = 0, loc6
	cmp.ne p9,p0 = 0, loc7 ;;

(p7)	st8 [loc5] = r9
(p8)	st8 [loc6] = r10
(p9)	st8 [loc7] = r11

	mov gp = loc1
	mov rp = loc2

	adds sp = STACK_SCRATCH_AREA, sp ;;

	mov ar.pfs = loc0
	br.ret.sptk.many rp
FUNCTION_END(sal_call)

