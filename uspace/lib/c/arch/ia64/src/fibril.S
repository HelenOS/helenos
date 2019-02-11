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
	alloc loc0 = ar.pfs, 1, 49, 0, 0
	mov loc1 = ar.unat ;;
	mov loc3 = ar.rsc

	.auto

	/*
	 * Flush dirty registers to backing store.
	 * After this ar.bsp and ar.bspstore are equal.
	 */
	flushrs
	mov loc4 = ar.bsp

	/*
	 * Put RSE to enforced lazy mode.
	 * So that ar.rnat can be read.
	 */
	and loc5 = ~3, loc3
	mov ar.rsc = loc5
	mov loc5 = ar.rnat

	.explicit

	mov loc6 = ar.lc

	add loc8 = __CONTEXT_OFFSET_AR_PFS, in0
	add loc9 = __CONTEXT_OFFSET_AR_UNAT_CALLER, in0
	add loc10 = __CONTEXT_OFFSET_AR_UNAT_CALLEE, in0
	add loc11 = __CONTEXT_OFFSET_AR_RSC, in0
	add loc12 = __CONTEXT_OFFSET_BSP, in0
	add loc13 = __CONTEXT_OFFSET_AR_RNAT, in0
	add loc14 = __CONTEXT_OFFSET_AR_LC, in0

	add loc15 = __CONTEXT_OFFSET_R1, in0
	add loc16 = __CONTEXT_OFFSET_R4, in0
	add loc17 = __CONTEXT_OFFSET_R5, in0
	add loc18 = __CONTEXT_OFFSET_R6, in0
	add loc19 = __CONTEXT_OFFSET_R7, in0
	add loc20 = __CONTEXT_OFFSET_SP, in0
	add loc21 = __CONTEXT_OFFSET_TP, in0

	add loc22 = __CONTEXT_OFFSET_PC, in0
	add loc23 = __CONTEXT_OFFSET_B1, in0
	add loc24 = __CONTEXT_OFFSET_B2, in0
	add loc25 = __CONTEXT_OFFSET_B3, in0
	add loc26 = __CONTEXT_OFFSET_B4, in0
	add loc27 = __CONTEXT_OFFSET_B5, in0

	add loc28 = __CONTEXT_OFFSET_PR, in0

	add loc29 = __CONTEXT_OFFSET_F2, in0
	add loc30 = __CONTEXT_OFFSET_F3, in0
	add loc31 = __CONTEXT_OFFSET_F4, in0
	add loc32 = __CONTEXT_OFFSET_F5, in0

	add loc33 = __CONTEXT_OFFSET_F16, in0
	add loc34 = __CONTEXT_OFFSET_F17, in0
	add loc35 = __CONTEXT_OFFSET_F18, in0
	add loc36 = __CONTEXT_OFFSET_F19, in0
	add loc37 = __CONTEXT_OFFSET_F20, in0
	add loc38 = __CONTEXT_OFFSET_F21, in0
	add loc39 = __CONTEXT_OFFSET_F22, in0
	add loc40 = __CONTEXT_OFFSET_F23, in0
	add loc41 = __CONTEXT_OFFSET_F24, in0
	add loc42 = __CONTEXT_OFFSET_F25, in0
	add loc43 = __CONTEXT_OFFSET_F26, in0
	add loc44 = __CONTEXT_OFFSET_F27, in0
	add loc45 = __CONTEXT_OFFSET_F28, in0
	add loc46 = __CONTEXT_OFFSET_F29, in0
	add loc47 = __CONTEXT_OFFSET_F30, in0
	add loc48 = __CONTEXT_OFFSET_F31, in0 ;;

	/*
	 * Save general registers including NaT bits
	 */
	st8.spill [loc15] = r1 ;;
	st8.spill [loc16] = r4 ;;
	st8.spill [loc17] = r5 ;;
	st8.spill [loc18] = r6 ;;
	st8.spill [loc19] = r7 ;;
	st8.spill [loc20] = r12	;;	/* save sp */
	st8.spill [loc21] = r13 ;;	/* save tp */

	mov loc2 = ar.unat

	/*
	 * Save application registers
	 */
	st8 [loc8] = loc0	/* save ar.pfs */
	st8 [loc9] = loc1 ;;	/* save ar.unat (caller) */
	st8 [loc10] = loc2	/* save ar.unat (callee) */
	st8 [loc11] = loc3	/* save ar.rsc */
	st8 [loc12] = loc4	/* save ar.bsp */
	st8 [loc13] = loc5	/* save ar.rnat */
	st8 [loc14] = loc6 ;;	/* save ar.lc */

	/*
	 * Save branch registers
	 */
	mov loc2 = b0
	mov loc3 = b1
	mov loc4 = b2
	mov loc5 = b3
	mov loc6 = b4
	mov loc7 = b5 ;;
	st8 [loc22] = loc2	/* save pc */
	st8 [loc23] = loc3
	st8 [loc24] = loc4
	st8 [loc25] = loc5
	st8 [loc26] = loc6
	st8 [loc27] = loc7 ;;

	/*
	 * Save predicate registers
	 */
	mov loc2 = pr ;;
	st8 [loc28] = loc2

	/*
	 * Save floating-point registers.
	 */
	stf.spill [loc29] = f2
	stf.spill [loc30] = f3
	stf.spill [loc31] = f4
	stf.spill [loc32] = f5

	stf.spill [loc33] = f16
	stf.spill [loc34] = f17
	stf.spill [loc35] = f18
	stf.spill [loc36] = f19
	stf.spill [loc37] = f20
	stf.spill [loc38] = f21
	stf.spill [loc39] = f22
	stf.spill [loc40] = f23
	stf.spill [loc41] = f24
	stf.spill [loc42] = f25
	stf.spill [loc43] = f26
	stf.spill [loc44] = f27
	stf.spill [loc45] = f28
	stf.spill [loc46] = f29
	stf.spill [loc47] = f30
	stf.spill [loc48] = f31

	mov ar.unat = loc1

	mov r8 = 0 	/* __context_save returns 0 */
	br.ret.sptk.many b0
FUNCTION_END(__context_save)

FUNCTION_BEGIN(__context_restore)
	alloc loc0 = ar.pfs, 2, 50, 0, 0 ;;

	add loc9 = __CONTEXT_OFFSET_AR_PFS, in0
	add loc10 = __CONTEXT_OFFSET_AR_UNAT_CALLER, in0
	add loc11 = __CONTEXT_OFFSET_AR_UNAT_CALLEE, in0
	add loc12 = __CONTEXT_OFFSET_AR_RSC, in0
	add loc13 = __CONTEXT_OFFSET_BSP, in0
	add loc14 = __CONTEXT_OFFSET_AR_RNAT, in0
	add loc15 = __CONTEXT_OFFSET_AR_LC, in0

	add loc16 = __CONTEXT_OFFSET_R1, in0
	add loc17 = __CONTEXT_OFFSET_R4, in0
	add loc18 = __CONTEXT_OFFSET_R5, in0
	add loc19 = __CONTEXT_OFFSET_R6, in0
	add loc20 = __CONTEXT_OFFSET_R7, in0
	add loc21 = __CONTEXT_OFFSET_SP, in0
	add loc22 = __CONTEXT_OFFSET_TP, in0

	add loc23 = __CONTEXT_OFFSET_PC, in0
	add loc24 = __CONTEXT_OFFSET_B1, in0
	add loc25 = __CONTEXT_OFFSET_B2, in0
	add loc26 = __CONTEXT_OFFSET_B3, in0
	add loc27 = __CONTEXT_OFFSET_B4, in0
	add loc28 = __CONTEXT_OFFSET_B5, in0

	add loc29 = __CONTEXT_OFFSET_PR, in0

	add loc30 = __CONTEXT_OFFSET_F2, in0
	add loc31 = __CONTEXT_OFFSET_F3, in0
	add loc32 = __CONTEXT_OFFSET_F4, in0
	add loc33 = __CONTEXT_OFFSET_F5, in0

	add loc34 = __CONTEXT_OFFSET_F16, in0
	add loc35 = __CONTEXT_OFFSET_F17, in0
	add loc36 = __CONTEXT_OFFSET_F18, in0
	add loc37 = __CONTEXT_OFFSET_F19, in0
	add loc38 = __CONTEXT_OFFSET_F20, in0
	add loc39 = __CONTEXT_OFFSET_F21, in0
	add loc40 = __CONTEXT_OFFSET_F22, in0
	add loc41 = __CONTEXT_OFFSET_F23, in0
	add loc42 = __CONTEXT_OFFSET_F24, in0
	add loc43 = __CONTEXT_OFFSET_F25, in0
	add loc44 = __CONTEXT_OFFSET_F26, in0
	add loc45 = __CONTEXT_OFFSET_F27, in0
	add loc46 = __CONTEXT_OFFSET_F28, in0
	add loc47 = __CONTEXT_OFFSET_F29, in0
	add loc48 = __CONTEXT_OFFSET_F30, in0
	add loc49 = __CONTEXT_OFFSET_F31, in0 ;;

	ld8 loc0 = [loc9]	/* load ar.pfs */
	ld8 loc1 = [loc10]	/* load ar.unat (caller) */
	ld8 loc2 = [loc11]	/* load ar.unat (callee) */
	ld8 loc3 = [loc12]	/* load ar.rsc */
	ld8 loc4 = [loc13]	/* load ar.bsp */
	ld8 loc5 = [loc14]	/* load ar.rnat */
	ld8 loc6 = [loc15]	/* load ar.lc */

	.auto

	/*
	 * Invalidate the ALAT
	 */
	invala

	/*
	 * Put RSE to enforced lazy mode.
	 * So that ar.bspstore and ar.rnat can be written.
	 */
	movl loc8 = ~3
	and loc8 = loc3, loc8
	mov ar.rsc = loc8

	/*
	 * Flush dirty registers to backing store.
	 * We do this because we want the following move
	 * to ar.bspstore to assign the same value to ar.bsp.
	 */
	flushrs

	/*
	 * Restore application registers
	 */
	mov ar.bspstore = loc4	/* rse.bspload = ar.bsp = ar.bspstore = loc4 */
	mov ar.rnat = loc5
	mov ar.pfs = loc0
	mov ar.rsc = loc3

	.explicit

	mov ar.unat = loc2 ;;
	mov ar.lc = loc6

	/*
	 * Restore general registers including NaT bits
	 */
	ld8.fill r1 = [loc16] ;;
	ld8.fill r4 = [loc17] ;;
	ld8.fill r5 = [loc18] ;;
	ld8.fill r6 = [loc19] ;;
	ld8.fill r7 = [loc20] ;;
	ld8.fill r12 = [loc21] ;;	/* restore sp */
	ld8.fill r13 = [loc22] ;;

	/*
	 * Restore branch registers
	 */
	ld8 loc2 = [loc23]		/* restore pc */
	ld8 loc3 = [loc24]
	ld8 loc4 = [loc25]
	ld8 loc5 = [loc26]
	ld8 loc6 = [loc27]
	ld8 loc7 = [loc28] ;;
	mov b0 = loc2
	mov b1 = loc3
	mov b2 = loc4
	mov b3 = loc5
	mov b4 = loc6
	mov b5 = loc7 ;;

	/*
	 * Restore predicate registers
	 */
	ld8 loc2 = [loc29] ;;
	mov pr = loc2, ~0

	/*
	 * Restore floating-point registers.
	 */
	ldf.fill f2 = [loc30]
	ldf.fill f3 = [loc31]
	ldf.fill f4 = [loc32]
	ldf.fill f5 = [loc33]

	ldf.fill f16 = [loc34]
	ldf.fill f17 = [loc35]
	ldf.fill f18 = [loc36]
	ldf.fill f19 = [loc37]
	ldf.fill f20 = [loc38]
	ldf.fill f21 = [loc39]
	ldf.fill f22 = [loc40]
	ldf.fill f23 = [loc41]
	ldf.fill f24 = [loc42]
	ldf.fill f25 = [loc43]
	ldf.fill f26 = [loc44]
	ldf.fill f27 = [loc45]
	ldf.fill f28 = [loc46]
	ldf.fill f29 = [loc47]
	ldf.fill f30 = [loc48]
	ldf.fill f31 = [loc49]

	mov ar.unat = loc1

	mov r8 = in1			/* __context_restore returns second argument */
	br.ret.sptk.many b0
FUNCTION_END(__context_restore)
