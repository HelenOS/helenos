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

.text

.global context_save
.global context_restore

context_save:
	alloc loc0 = ar.pfs, 1, 8, 0, 0
	mov loc1 = ar.unat	;;
	/* loc2 */
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
	
	/*
	 * Save application registers
	 */
	st8 [in0] = loc0, 8	;;	/* save ar.pfs */
	st8 [in0] = loc1, 8	;;	/* save ar.unat (caller) */
	mov loc2 = in0		;;
	add in0 = 8, in0	;;	/* skip ar.unat (callee) */
	st8 [in0] = loc3, 8	;;	/* save ar.rsc */
	st8 [in0] = loc4, 8	;;	/* save ar.bsp */
	st8 [in0] = loc5, 8	;;	/* save ar.rnat */
	st8 [in0] = loc6, 8	;;	/* save ar.lc */
	
	/*
	 * Save general registers including NaT bits
	 */
	st8.spill [in0] = r1, 8		;;
	st8.spill [in0] = r4, 8		;;
	st8.spill [in0] = r5, 8		;;
	st8.spill [in0] = r6, 8		;;
	st8.spill [in0] = r7, 8		;;
	st8.spill [in0] = r12, 8	;;	/* save sp */
	st8.spill [in0] = r13, 8	;;	/* save tp */

	mov loc3 = ar.unat		;;
	st8 [loc2] = loc3		/* save ar.unat (callee) */

	/*
	 * Save branch registers
	 */
	mov loc2 = b0		;;
	st8 [in0] = loc2, 8		/* save pc */
	mov loc3 = b1		;;
	st8 [in0] = loc3, 8
	mov loc4 = b2		;;
	st8 [in0] = loc4, 8
	mov loc5 = b3		;;
	st8 [in0] = loc5, 8
	mov loc6 = b4		;;
	st8 [in0] = loc6, 8
	mov loc7 = b5		;;
	st8 [in0] = loc7, 8

	/*
	 * Save predicate registers
	 */
	mov loc2 = pr		;;
	st8 [in0] = loc2, 16;; 		/* Next fpu registers should be spilled to 16B aligned address */

	/*
	 * Save floating-point registers.
	 */
	stf.spill [in0] = f2, 16 ;;
	stf.spill [in0] = f3, 16 ;;
	stf.spill [in0] = f4, 16 ;;
	stf.spill [in0] = f5, 16 ;;

	stf.spill [in0] = f16, 16 ;;
	stf.spill [in0] = f17, 16 ;;
	stf.spill [in0] = f18, 16 ;;
	stf.spill [in0] = f19, 16 ;;
	stf.spill [in0] = f20, 16 ;;
	stf.spill [in0] = f21, 16 ;;
	stf.spill [in0] = f22, 16 ;;
	stf.spill [in0] = f23, 16 ;;
	stf.spill [in0] = f24, 16 ;;
	stf.spill [in0] = f25, 16 ;;
	stf.spill [in0] = f26, 16 ;;
	stf.spill [in0] = f27, 16 ;;
	stf.spill [in0] = f28, 16 ;;
	stf.spill [in0] = f29, 16 ;;
	stf.spill [in0] = f30, 16 ;;
	stf.spill [in0] = f31, 16 ;;	

	mov ar.unat = loc1
	
	add r8 = r0, r0, 1 		/* context_save returns 1 */
	br.ret.sptk.many b0

context_restore:
	alloc loc0 = ar.pfs, 1, 9, 0, 0	;;

	ld8 loc0 = [in0], 8	;;	/* load ar.pfs */
	ld8 loc1 = [in0], 8	;;	/* load ar.unat (caller) */
	ld8 loc2 = [in0], 8	;;	/* load ar.unat (callee) */
	ld8 loc3 = [in0], 8	;;	/* load ar.rsc */
	ld8 loc4 = [in0], 8	;;	/* load ar.bsp */
	ld8 loc5 = [in0], 8	;;	/* load ar.rnat */
	ld8 loc6 = [in0], 8	;;	/* load ar.lc */
	
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

	mov ar.unat = loc2	;;
	mov ar.lc = loc6
	
	/*
	 * Restore general registers including NaT bits
	 */
	ld8.fill r1 = [in0], 8	;;
	ld8.fill r4 = [in0], 8	;;
	ld8.fill r5 = [in0], 8	;;
	ld8.fill r6 = [in0], 8	;;
	ld8.fill r7 = [in0], 8	;;
	ld8.fill r12 = [in0], 8	;;	/* restore sp */
	ld8.fill r13 = [in0], 8	;;

	/* 
	 * Restore branch registers
	 */
	ld8 loc2 = [in0], 8	;;	/* restore pc */
	mov b0 = loc2
	ld8 loc3 = [in0], 8	;;
	mov b1 = loc3
	ld8 loc4 = [in0], 8	;;
	mov b2 = loc4
	ld8 loc5 = [in0], 8	;;
	mov b3 = loc5
	ld8 loc6 = [in0], 8	;;
	mov b4 = loc6
	ld8 loc7 = [in0], 8	;;
	mov b5 = loc7

	/*
	 * Restore predicate registers
	 */
	ld8 loc2 = [in0], 16	;;
	mov pr = loc2, ~0

	/*
	 * Restore floating-point registers.
	 */
	ldf.fill f2 = [in0], 16 ;;
	ldf.fill f3 = [in0], 16 ;;
	ldf.fill f4 = [in0], 16 ;;
	ldf.fill f5 = [in0], 16 ;;

	ldf.fill f16 = [in0], 16 ;;
	ldf.fill f17 = [in0], 16 ;;
	ldf.fill f18 = [in0], 16 ;;
	ldf.fill f19 = [in0], 16 ;;
	ldf.fill f20 = [in0], 16 ;;
	ldf.fill f21 = [in0], 16 ;;
	ldf.fill f22 = [in0], 16 ;;
	ldf.fill f23 = [in0], 16 ;;
	ldf.fill f24 = [in0], 16 ;;
	ldf.fill f25 = [in0], 16 ;;
	ldf.fill f26 = [in0], 16 ;;
	ldf.fill f27 = [in0], 16 ;;
	ldf.fill f28 = [in0], 16 ;;
	ldf.fill f29 = [in0], 16 ;;
	ldf.fill f30 = [in0], 16 ;;
	ldf.fill f31 = [in0], 16 ;;
	
	mov ar.unat = loc1
	
	mov r8 = r0			/* context_restore returns 0 */
	br.ret.sptk.many b0
