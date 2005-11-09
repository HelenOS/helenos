#
# Copyright (C) 2005 Jakub Vana
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

#include <arch/stack.h>
#include <arch/register.h>

#define STACK_ITEMS		12
#define STACK_FRAME_SIZE	((STACK_ITEMS*STACK_ITEM_SIZE) + STACK_SCRATCH_AREA_SIZE)

#if (STACK_FRAME_SIZE % STACK_ALIGNMENT != 0)
#error Memory stack must be 16-byte aligned.
#endif

/** Heavyweight interrupt handler
 *
 * This macro roughly follows steps from 1 to 19 described in
 * Intel Itanium Architecture Software Developer's Manual, Chapter 3.4.2.
 *
 * HEAVYWEIGHT_HANDLER macro must cram into 16 bundles (48 instructions).
 * This goal is achieved by using procedure calls after RSE becomes operational.
 *
 * Some steps are skipped (enabling and disabling interrupts).
 * Some steps are not fully supported yet (e.g. interruptions
 * from userspace and floating-point context).
 *
 * @param offs Offset from the beginning of IVT.
 * @param handler Interrupt handler address.
 */
.macro HEAVYWEIGHT_HANDLER offs, handler=universal_handler
    .org ivt + \offs
	mov r24 = \offs
	movl r25 = \handler ;;
	mov ar.k0 = r24
	mov ar.k1 = r25
	br heavyweight_handler
.endm

.global heavyweight_handler
heavyweight_handler:
    /* 1. copy interrupt registers into bank 0 */
	mov r24 = cr.iip
	mov r25 = cr.ipsr
	mov r26 = cr.iipa
	mov r27 = cr.isr
	mov r28 = cr.ifa
	
    /* 2. preserve predicate register into bank 0 */
	mov r29 = pr ;;
	
    /* 3. switch to kernel memory stack */
	/* TODO: support interruptions from userspace */
	/* assume kernel stack */
	
	add r31 = -8, r12 ;;
	add r12 = -STACK_FRAME_SIZE, r12

    /* 4. save registers in bank 0 into memory stack */	
	st8 [r31] = r29, -8 ;;	/* save predicate registers */

	st8 [r31] = r24, -8 ;;	/* save cr.iip */
	st8 [r31] = r25, -8 ;;	/* save cr.ipsr */
	st8 [r31] = r26, -8 ;;	/* save cr.iipa */
	st8 [r31] = r27, -8 ;;	/* save cr.isr */
	st8 [r31] = r28, -8	/* save cr.ifa */		

    /* 5. RSE switch from interrupted context */
	mov r24 = ar.rsc
	mov r25 = ar.pfs
	cover
	mov r26 = cr.ifs
	
	st8 [r31] = r24, -8;;	/* save ar.rsc */
	st8 [r31] = r25, -8;;	/* save ar.pfs */
	st8 [r31] = r26, -8	/* save ar.ifs */
	
	and r30 = ~3, r24 ;;
	mov ar.rsc = r30 ;;	/* place RSE in enforced lazy mode */
	
	mov r27 = ar.rnat
	mov r28 = ar.bspstore ;;
	
	/* assume kernel backing store */
	/* mov ar.bspstore = r28 ;; */
	
	mov r29 = ar.bsp
	
	st8 [r31] = r27, -8 ;;	/* save ar.rnat */
	st8 [r31] = r28, -8 ;;	/* save ar.bspstore */
	st8 [r31] = r29, -8	/* save ar.bsp */
	
	mov ar.rsc = r24	/* restore RSE's setting */
	
    /* steps 6 - 15 are done by heavyweight_handler_inner() */
	mov r24 = b0 		/* save b0 belonging to interrupted context */
	mov r26 = ar.k0
	mov r25 = ar.k1
	br.call.sptk.many rp = heavyweight_handler_inner
0:	mov b0 = r24		/* restore b0 belonging to the interrupted context */

    /* 16. RSE switch to interrupted context */
	cover			/* allocate zerro size frame (step 1 (from Intel Docs)) */

	add r31 = STACK_SCRATCH_AREA_SIZE, r12 ;;

	mov r28 = ar.bspstore   	/* calculate loadrs (step 2) */
	ld8 r29 = [r31], +8 ;;		/* load ar.bsp */
	sub r27 = r29 , r28 ;;
	shl r27 = r27, 16

	mov r24 = ar.rsc ;;
	and r30 = ~3, r24 ;;
	or  r24 = r30 , r27 ;;     
	mov ar.rsc = r24 ;;		/* place RSE in enforced lazy mode */

	loadrs 				/* (step 3) */

	ld8 r28 = [r31], +8 ;;    	/* load ar.bspstore */
	ld8 r27 = [r31], +8 ;;		/* load ar.rnat */
	ld8 r26 = [r31], +8 ;;		/* load cr.ifs */
	ld8 r25 = [r31], +8 ;;		/* load ar.pfs */
	ld8 r24 = [r31], +8 ;;		/* load ar.rsc */

	/* mov ar.bspstore = r28 ;; */	/* (step 4) */
	/* mov ar.rnat = r27 */		/* (step 5) */

	mov ar.pfs = r25		/* (step 6) */
	mov cr.ifs = r26	

	mov ar.rsc = r24		/* (step 7) */

    /* 17. restore interruption state from memory stack */
	ld8 r28 = [r31], +8 ;;	/* load cr.ifa */		
	ld8 r27 = [r31], +8 ;;	/* load cr.isr */
	ld8 r26 = [r31], +8 ;;	/* load cr.iipa */
	ld8 r25 = [r31], +8 ;;	/* load cr.ipsr */
	ld8 r24 = [r31], +8 ;;	/* load cr.iip */

	mov cr.iip = r24
	mov cr.ipsr = r25
	mov cr.iipa = r26
	mov cr.isr = r27
	mov cr.ifa = r28

    /* 18. restore predicate registers from memory stack */
	ld8 r29 = [r31] , -8 ;;	/* load predicate registers */
	mov pr = r29
	
    /* 19. return from interruption */
    	add r12 = STACK_FRAME_SIZE, r12
	rfi ;;

.global heavyweight_handler_inner
heavyweight_handler_inner:
	/*
	 * From this point, the rest of the interrupted context
	 * will be preserved in stacked registers and backing store.
	 */
	alloc loc0 = ar.pfs, 0, 47, 2, 0 ;;
	
	/* bank 0 is going to be shadowed, copy essential data from there */
	mov loc1 = r24	/* b0 belonging to interrupted context */
	mov loc2 = r25
	mov out0 = r26
	
	add out1 = STACK_SCRATCH_AREA_SIZE, r12

    /* 6. switch to bank 1 and reenable PSR.ic */
	ssm PSR_IC_MASK
	bsw.1 ;;
	srlz.d
	
    /* 7. preserve branch and application registers */
    	mov loc3 = ar.unat
	mov loc4 = ar.lc
	mov loc5 = ar.ec
	mov loc6 = ar.ccv
	mov loc7 = ar.csd
	mov loc8 = ar.ssd
	
	mov loc9 = b0
	mov loc10 = b1
	mov loc11 = b2
	mov loc12 = b3
	mov loc13 = b4
	mov loc14 = b5
	mov loc15 = b6
	mov loc16 = b7
	
    /* 8. preserve general and floating-point registers */
	/* TODO: save floating-point context */
	mov loc17 = r1
	mov loc18 = r2
	mov loc19 = r3
	mov loc20 = r4
	mov loc21 = r5
	mov loc22 = r6
	mov loc23 = r7
	mov loc24 = r8
	mov loc25 = r9
	mov loc26 = r10
	mov loc27 = r11
	/* skip r12 (stack pointer) */
	mov loc28 = r13
	mov loc29 = r14
	mov loc30 = r15
	mov loc31 = r16
	mov loc32 = r17
	mov loc33 = r18
	mov loc34 = r19
	mov loc35 = r20
	mov loc36 = r21
	mov loc37 = r22
	mov loc38 = r23
	mov loc39 = r24
	mov loc40 = r25
	mov loc41 = r26
	mov loc42 = r27
	mov loc43 = r28
	mov loc44 = r29
	mov loc45 = r30
	mov loc46 = r31
    
    /* 9. skipped (will not enable interrupts) */
	/*
    	 * ssm PSR_I_MASK
	 * ;;
	 * srlz.d
	 */

    /* 10. call handler */
    	mov b1 = loc2
	br.call.sptk.many b0 = b1

    /* 11. return from handler */
0:
	
    /* 12. skipped (will not disable interrupts) */
	/*
    	 * rsm PSR_I_MASK
	 * ;;
	 * srlz.d
	 */

    /* 13. restore general and floating-point registers */
	/* TODO: restore floating-point context */
	mov r1 = loc17
	mov r2 = loc18
	mov r3 = loc19
	mov r4 = loc20
	mov r5 = loc21
	mov r6 = loc22
	mov r7 = loc23
	mov r8 = loc24
	mov r9 = loc25
	mov r10 = loc26
	mov r11 = loc27
	/* skip r12 (stack pointer) */
	mov r13 = loc28
	mov r14 = loc29
	mov r15 = loc30
	mov r16 = loc31
	mov r17 = loc32
	mov r18 = loc33
	mov r19 = loc34
	mov r20 = loc35
	mov r21 = loc36
	mov r22 = loc37
	mov r23 = loc38
	mov r24 = loc39
	mov r25 = loc40
	mov r26 = loc41 
	mov r27 = loc42
	mov r28 = loc43
	mov r29 = loc44
	mov r30 = loc45
	mov r31 = loc46
	
    /* 14. restore branch and application registers */
    	mov ar.unat = loc3
	mov ar.lc = loc4
	mov ar.ec = loc5
	mov ar.ccv = loc6
	mov ar.csd = loc7
	mov ar.ssd = loc8
	
	mov b0 = loc9
	mov b1 = loc10
	mov b2 = loc11
	mov b3 = loc12
	mov b4 = loc13
	mov b5 = loc14
	mov b6 = loc15
	mov b7 = loc16
	
    /* 15. disable PSR.ic and switch to bank 0 */
	rsm PSR_IC_MASK
	bsw.0 ;;
	srlz.d

	mov r24 = loc1
	mov ar.pfs = loc0
	br.ret.sptk.many b0

.global ivt
.align 32768
ivt:
	HEAVYWEIGHT_HANDLER 0x0000
	HEAVYWEIGHT_HANDLER 0x0400
	HEAVYWEIGHT_HANDLER 0x0800
	HEAVYWEIGHT_HANDLER 0x0c00
	HEAVYWEIGHT_HANDLER 0x1000
	HEAVYWEIGHT_HANDLER 0x1400
	HEAVYWEIGHT_HANDLER 0x1800
	HEAVYWEIGHT_HANDLER 0x1c00
	HEAVYWEIGHT_HANDLER 0x2000
	HEAVYWEIGHT_HANDLER 0x2400
	HEAVYWEIGHT_HANDLER 0x2800
	HEAVYWEIGHT_HANDLER 0x2c00 break_instruction
	HEAVYWEIGHT_HANDLER 0x3000 external_interrupt	/* For external interrupt, heavyweight handler is used. */
	HEAVYWEIGHT_HANDLER 0x3400
	HEAVYWEIGHT_HANDLER 0x3800
	HEAVYWEIGHT_HANDLER 0x3c00
	HEAVYWEIGHT_HANDLER 0x4000
	HEAVYWEIGHT_HANDLER 0x4400
	HEAVYWEIGHT_HANDLER 0x4800
	HEAVYWEIGHT_HANDLER 0x4c00

	HEAVYWEIGHT_HANDLER 0x5000
	HEAVYWEIGHT_HANDLER 0x5100
	HEAVYWEIGHT_HANDLER 0x5200
	HEAVYWEIGHT_HANDLER 0x5300
	HEAVYWEIGHT_HANDLER 0x5400 general_exception
	HEAVYWEIGHT_HANDLER 0x5500
	HEAVYWEIGHT_HANDLER 0x5600
	HEAVYWEIGHT_HANDLER 0x5700
	HEAVYWEIGHT_HANDLER 0x5800
	HEAVYWEIGHT_HANDLER 0x5900
	HEAVYWEIGHT_HANDLER 0x5a00
	HEAVYWEIGHT_HANDLER 0x5b00
	HEAVYWEIGHT_HANDLER 0x5c00
	HEAVYWEIGHT_HANDLER 0x5d00
	HEAVYWEIGHT_HANDLER 0x5e00
	HEAVYWEIGHT_HANDLER 0x5f00
	
	HEAVYWEIGHT_HANDLER 0x6000
	HEAVYWEIGHT_HANDLER 0x6100
	HEAVYWEIGHT_HANDLER 0x6200
	HEAVYWEIGHT_HANDLER 0x6300
	HEAVYWEIGHT_HANDLER 0x6400
	HEAVYWEIGHT_HANDLER 0x6500
	HEAVYWEIGHT_HANDLER 0x6600
	HEAVYWEIGHT_HANDLER 0x6700
	HEAVYWEIGHT_HANDLER 0x6800
	HEAVYWEIGHT_HANDLER 0x6900
	HEAVYWEIGHT_HANDLER 0x6a00
	HEAVYWEIGHT_HANDLER 0x6b00
	HEAVYWEIGHT_HANDLER 0x6c00
	HEAVYWEIGHT_HANDLER 0x6d00
	HEAVYWEIGHT_HANDLER 0x6e00
	HEAVYWEIGHT_HANDLER 0x6f00

	HEAVYWEIGHT_HANDLER 0x7000
	HEAVYWEIGHT_HANDLER 0x7100
	HEAVYWEIGHT_HANDLER 0x7200
	HEAVYWEIGHT_HANDLER 0x7300
	HEAVYWEIGHT_HANDLER 0x7400
	HEAVYWEIGHT_HANDLER 0x7500
	HEAVYWEIGHT_HANDLER 0x7600
	HEAVYWEIGHT_HANDLER 0x7700
	HEAVYWEIGHT_HANDLER 0x7800
	HEAVYWEIGHT_HANDLER 0x7900
	HEAVYWEIGHT_HANDLER 0x7a00
	HEAVYWEIGHT_HANDLER 0x7b00
	HEAVYWEIGHT_HANDLER 0x7c00
	HEAVYWEIGHT_HANDLER 0x7d00
	HEAVYWEIGHT_HANDLER 0x7e00
	HEAVYWEIGHT_HANDLER 0x7f00
