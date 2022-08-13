/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

