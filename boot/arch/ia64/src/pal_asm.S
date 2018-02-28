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

FUNCTION_BEGIN(pal_static_call)
	alloc loc0 = ar.pfs, 7, 6, 0, 0

	mov loc1 = psr ;;
	mov psr.l = r0 ;;
	srlz.i
	srlz.d

	mov loc2 = gp
	mov loc3 = rp

	addl loc4 = @gprel(pal_proc), gp

	mov r28 = in0
	mov r29 = in1
	mov r30 = in2
	mov r31 = in3 ;;

	ld8 loc4 = [loc4]
	movl loc5 = 0f ;;

	mov b6 = loc4
	mov rp = loc5 ;;
	br.cond.sptk.many b6

0:
	mov psr.l = loc1 ;;
	srlz.i
	srlz.d

	cmp.ne p7,p0 = 0, in4
	cmp.ne p8,p0 = 0, in5
	cmp.ne p9,p0 = 0, in6 ;;

(p7)	st8 [in4] = r9
(p8)	st8 [in5] = r10
(p9)	st8 [in6] = r11

	mov gp = loc2
	mov rp = loc3 ;;

	mov ar.pfs = loc0
	br.ret.sptk.many rp
FUNCTION_END(pal_static_call)

