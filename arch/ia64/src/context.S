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
	movl loc5 = ~3
	and loc5 = loc3, loc5
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
	st8.spill [in0] = r13, 8	;;

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
	st8 [in0] = loc2, 8
	
	mov ar.unat = loc1
	
	add r8 = r0, r0, 1 		/* context_save returns 1 */
	br.ret.sptk.many b0

context_restore:
	alloc loc0 = ar.pfs, 1, 8, 0, 0	;;

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
	 * Restore application registers
	 */

	/* TODO: ensure RSE lazy mode */
	mov ar.bspstore = loc4
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
	ld8 loc2 = [in0], 8	;;
	mov pr = loc2, ~0
	
	mov ar.unat = loc1
	
	mov r8 = r0			/* context_restore returns 0 */
	br.ret.sptk.many b0
