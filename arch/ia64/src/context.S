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
	alloc loc0 = ar.pfs, 1, 11, 0, 0
	mov loc1 = ar.unat	;;
	/* loc2 */
	mov loc3 = ar.rsc
	mov loc4 = ar.bsp
	mov loc5 = ar.rnat
	mov loc6 = ar.lc
	mov loc7 = ar.ec
	mov loc8 = ar.ccv
	mov loc9 = ar.csd
	mov loc10 = ar.ssd
	
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
	st8 [in0] = loc7, 8	;;	/* save ar.ec */
	st8 [in0] = loc8, 8	;;	/* save ar.ccv */
	st8 [in0] = loc9, 8	;;	/* save ar.csd */
	st8 [in0] = loc10, 8	;;	/* save ar.ssd */	
	
	/*
	 * Save general registers including NaT bits
	 */
	st8.spill [in0] = r1, 8		;;
	st8.spill [in0] = r2, 8		;;
	st8.spill [in0] = r3, 8		;;
	st8.spill [in0] = r4, 8		;;
	st8.spill [in0] = r5, 8		;;
	st8.spill [in0] = r6, 8		;;
	st8.spill [in0] = r7, 8		;;
	st8.spill [in0] = r8, 8		;;
	st8.spill [in0] = r9, 8		;;
	st8.spill [in0] = r10, 8	;;
	st8.spill [in0] = r11, 8	;;
	st8.spill [in0] = r12, 8	;;	/* save sp */
	st8.spill [in0] = r13, 8	;;
	st8.spill [in0] = r14, 8	;;
	st8.spill [in0] = r15, 8	;;
	st8.spill [in0] = r16, 8	;;
	st8.spill [in0] = r17, 8	;;
	st8.spill [in0] = r18, 8	;;
	st8.spill [in0] = r19, 8	;;
	st8.spill [in0] = r20, 8	;;
	st8.spill [in0] = r21, 8	;;
	st8.spill [in0] = r22, 8	;;
	st8.spill [in0] = r23, 8	;;
	st8.spill [in0] = r24, 8	;;
	st8.spill [in0] = r25, 8	;;
	st8.spill [in0] = r26, 8	;;
	st8.spill [in0] = r27, 8	;;
	st8.spill [in0] = r28, 8	;;
	st8.spill [in0] = r29, 8	;;
	st8.spill [in0] = r30, 8	;;
	st8.spill [in0] = r31, 8	;;

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
	mov loc8 = b6		;;
	st8 [in0] = loc8, 8
	mov loc9 = b7		;;
	st8 [in0] = loc9, 8

	/*
	 * Save predicate registers
	 */
	mov loc2 = pr		;;
	st8 [in0] = loc2, 8
	
	mov ar.pfs = loc0
	mov ar.unat = loc1
	
	add r8 = r0, r0, 1 		/* context_save returns 1 */
	br.ret.sptk.many b0

context_restore:
	alloc loc0 = ar.pfs, 1, 11, 0, 0	;;

	ld8 loc0 = [in0], 8	;;	/* load ar.pfs */
	ld8 loc1 = [in0], 8	;;	/* load ar.unat (caller) */
	ld8 loc2 = [in0], 8	;;	/* load ar.unat (callee) */
	ld8 loc3 = [in0], 8	;;	/* load ar.rsc */
	ld8 loc4 = [in0], 8	;;	/* load ar.bsp */
	ld8 loc5 = [in0], 8	;;	/* load ar.rnat */
	ld8 loc6 = [in0], 8	;;	/* load ar.lc */
	ld8 loc7 = [in0], 8	;;	/* load ar.ec */
	ld8 loc8 = [in0], 8	;;	/* load ar.ccv */
	ld8 loc9 = [in0], 8	;;	/* load ar.csd */
	ld8 loc10 = [in0], 8	;;	/* load ar.ssd */
	
	/*
	 * Restore application registers
	 */
	mov ar.unat = loc2	;;
	/* TODO: restore ar.rsc, ar.rnat, ar.bspstore */	
	mov ar.lc = loc6
	mov ar.ec = loc7
	mov ar.ccv = loc8
	mov ar.csd = loc9
	mov ar.ssd = loc10
	
	/*
	 * Restore general registers including NaT bits
	 */
	ld8.fill r1 = [in0], 8	;;
	ld8.fill r2 = [in0], 8	;;
	ld8.fill r3 = [in0], 8	;;
	ld8.fill r4 = [in0], 8	;;
	ld8.fill r5 = [in0], 8	;;
	ld8.fill r6 = [in0], 8	;;
	ld8.fill r7 = [in0], 8	;;
	ld8.fill r8 = [in0], 8	;;
	ld8.fill r9 = [in0], 8	;;
	ld8.fill r10 = [in0], 8	;;
	ld8.fill r11 = [in0], 8	;;
	ld8.fill r12 = [in0], 8	;;	/* restore sp */
	ld8.fill r13 = [in0], 8	;;
	ld8.fill r14 = [in0], 8	;;
	ld8.fill r15 = [in0], 8	;;
	ld8.fill r16 = [in0], 8	;;
	ld8.fill r17 = [in0], 8	;;
	ld8.fill r18 = [in0], 8	;;
	ld8.fill r19 = [in0], 8	;;
	ld8.fill r20 = [in0], 8	;;
	ld8.fill r21 = [in0], 8	;;
	ld8.fill r22 = [in0], 8	;;
	ld8.fill r23 = [in0], 8	;;
	ld8.fill r24 = [in0], 8	;;
	ld8.fill r25 = [in0], 8	;;
	ld8.fill r26 = [in0], 8	;;
	ld8.fill r27 = [in0], 8	;;
	ld8.fill r28 = [in0], 8	;;
	ld8.fill r29 = [in0], 8	;;
	ld8.fill r30 = [in0], 8	;;
	ld8.fill r31 = [in0], 8	;;

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
	ld8 loc8 = [in0], 8	;;
	mov b6 = loc8
	ld8 loc9 = [in0], 8	;;
	mov b7 = loc9

	/*
	 * Restore predicate registers
	 */
	ld8 loc2 = [in0], 8	;;
	mov pr = loc2, ~0
	
	mov ar.pfs = loc0
	mov ar.unat = loc1
	
	mov r8 = r0			/* context_restore returns 0 */
	br.ret.sptk.many b0
