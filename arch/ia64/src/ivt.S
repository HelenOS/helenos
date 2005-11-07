#
# Copyright (C) 2005 Jakub Vana
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
 */
.macro HEAVYWEIGHT_HANDLER offs handler
    .org IVT + \offs

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
	
    /* 4. save registers in bank 0 into memory stack */
	add r31 = -8, r12 ;;
	add r12 = -STACK_FRAME_SIZE, r12 ;;
	
	st8 [r31] = r29, -8 ;;	/* save predicate registers */

	st8 [r31] = r24, -8 ;;	/* save cr.iip */
	st8 [r31] = r25, -8 ;;	/* save cr.ipsr */
	st8 [r31] = r26, -8 ;;	/* save cr.iipa */
	st8 [r31] = r27, -8 ;;	/* save cr.isr */
	st8 [r31] = r28, -8 ;;	/* save cr.ifa */		

    /* 5. RSE switch from interrupted context */
    	.auto
	mov r24 = ar.rsc
	mov r25 = ar.pfs
	cover
	mov r26 = cr.ifs
	
	st8 [r31] = r24, -8	/* save ar.rsc */
	st8 [r31] = r25, -8	/* save ar.pfs */
	st8 [r31] = r26, -8	/* save ar.ifs */
	
	and r30 = ~3, r24
	mov ar.rsc = r30	/* place RSE in enforced lazy mode */
	
	mov r27 = ar.rnat
	mov r28 = ar.bspstore
	
	/* assume kernel backing store */
	mov ar.bspstore = r28
	
	mov r29 = ar.bsp
	
	st8 [r31] = r27, -8	/* save ar.rnat */
	st8 [r31] = r28, -8	/* save ar.bspstore */
	st8 [r31] = r29		/* save ar.bsp */
	
	mov ar.rsc = r24	/* restore RSE's setting */
	.explicit
	
    /* the rest of the save-handler can be kept outside IVT */

	movl r24 = \handler
	mov r25 = b0
	br.call.sptk.many rp = heavyweight_handler_inner
0:	mov b0 = r25	

	br heavyweight_handler_finalize
.endm

.global heavyweight_handler_inner
heavyweight_handler_inner:
	/*
	 * From this point, the rest of the interrupted context
	 * will be preserved in stacked registers and backing store.
	 */
	alloc loc0 = ar.pfs, 0, 46, 0, 0 ;;
	
	/* copy handler address (r24 from bank 0 will be invisible soon) */
	mov loc1 = r24

    /* 6. switch to bank 1 and reenable PSR.ic */
	ssm 0x2000
	bsw.1 ;;
	srlz.d
	
    /* 7. preserve branch and application registers */
    	mov loc2 = ar.unat
	mov loc3 = ar.lc
	mov loc4 = ar.ec
	mov loc5 = ar.ccv
	mov loc6 = ar.csd
	mov loc7 = ar.ssd
	
	mov loc8 = b0
	mov loc9 = b1
	mov loc10 = b2
	mov loc11 = b3
	mov loc12 = b4
	mov loc13 = b5
	mov loc14 = b6
	mov loc15 = b7
	
    /* 8. preserve general and floating-point registers */
	/* TODO: save floating-point context */
	mov loc16 = r1
	mov loc17 = r2
	mov loc18 = r3
	mov loc19 = r4
	mov loc20 = r5
	mov loc21 = r6
	mov loc22 = r7
	mov loc23 = r8
	mov loc24 = r9
	mov loc25 = r10
	mov loc26 = r11
	/* skip r12 (stack pointer) */
	mov loc27 = r13
	mov loc28 = r14
	mov loc29 = r15
	mov loc30 = r16
	mov loc31 = r17
	mov loc32 = r18
	mov loc33 = r19
	mov loc34 = r20
	mov loc35 = r21
	mov loc36 = r22
	mov loc37 = r23
	mov loc38 = r24
	mov loc39 = r25
	mov loc40 = r26
	mov loc41 = r27
	mov loc42 = r28
	mov loc43 = r29
	mov loc44 = r30
	mov loc45 = r31
    
    /* 9. skipped (will not enable interrupts) */

    /* 10. call handler */
    	mov b1 = loc1
	br.call.sptk.many b0 = b1

    /* 11. return from handler */
0:
	
    /* 12. skipped (will not disable interrupts) */

    /* 13. restore general and floating-point registers */
	/* TODO: restore floating-point context */
	mov r1 = loc16
	mov r2 = loc17
	mov r3 = loc18
	mov r4 = loc19
	mov r5 = loc20
	mov r6 = loc21
	mov r7 = loc22
	mov r8 = loc23
	mov r9 = loc24
	mov r10 = loc25
	mov r11 = loc26
	/* skip r12 (stack pointer) */
	mov r13 = loc27
	mov r14 = loc28
	mov r15 = loc29
	mov r16 = loc30
	mov r17 = loc31
	mov r18 = loc32
	mov r19 = loc33
	mov r20 = loc34
	mov r21 = loc35
	mov r22 = loc36
	mov r23 = loc37
	mov r24 = loc38
	mov r25 = loc39
	mov r26 = loc40
	mov r27 = loc41
	mov r28 = loc42
	mov r29 = loc43
	mov r30 = loc44
	mov r31 = loc45
	
    /* 14. restore branch and application registers */
    	mov ar.unat = loc2
	mov ar.lc = loc3
	mov ar.ec = loc4
	mov ar.ccv = loc5
	mov ar.csd = loc6
	mov ar.ssd = loc7
	
	mov b0 = loc8
	mov b1 = loc9
	mov b2 = loc10
	mov b3 = loc11
	mov b4 = loc12
	mov b5 = loc13
	mov b6 = loc14
	mov b7 = loc15
	
    /* 15. disable PSR.ic and switch to bank 0 */
	rsm 0x2000
	bsw.0 ;;
	srlz.d

	mov ar.pfs = loc0
	br.ret.sptk.many rp

.global heavyweight_handler_finalize
heavyweight_handler_finalize:
    /* 16. RSE switch to interrupted context */

/********************************************************************************************/



	.auto
	cover			/*Allocate zerro size frame (Step 1(from Intel Docs))*/

	add r31 = STACK_SCRATCH_AREA_SIZE, r12

	mov r28 = ar.bspstore   /*Calculate loadrs (step 2)*/
	ld8 r29 = [r31], +8     
	sub r27 = r29 , r28
	shl r27 = r27, 16

	mov r24 = ar.rsc
	and r30 = ~3, r24
	or  r24 = r30 , r27     
	mov ar.rsc = r24	/* place RSE in enforced lazy mode */



	loadrs 			/*(Step 3)*/


				/*Read saved registers*/
	ld8 r28 = [r31], +8     /*ar.bspstore*/
	ld8 r27 = [r31], +8 	/*ar.rnat*/
	ld8 r26 = [r31], +8 	/*cr.ifs*/
	ld8 r25 = [r31], +8 	/*ar.pfs*/
	ld8 r24 = [r31], +8 	/*ar.rsc*/


	mov ar.bspstore = r28	/*(Step 4)*/
	mov ar.rnat = r27	/*(Step 5)*/

	mov ar.pfs = r25	/*(Step 6)*/
	mov cr.ifs = r26	

	mov ar.rsc = r24	/*(Step 7)*/


	.explicit	


/********************************************************************************************/



    /* 17. restore interruption state from memory stack */

	ld8 r28 = [r31] , +8 ;;	/* load cr.ifa */		
	ld8 r27 = [r31] , +8 ;;	/* load cr.isr */
	ld8 r26 = [r31] , +8 ;;	/* load cr.iipa */
	ld8 r25 = [r31] , +8 ;;	/* load cr.ipsr */
	ld8 r24 = [r31] , +8 ;;	/* load cr.iip */


	mov cr.iip = r24
	mov cr.ipsr = r25
	mov cr.iipa = r26
	mov cr.isr = r27
	mov cr.ifa = r28


	
    /* 18. restore predicate registers from memory stack */


	ld8 r29 = [r31] , -8 ;;	/* load predicate registers */
	mov pr =r29 ;;
	
	add r12 = STACK_FRAME_SIZE,r12;;
	
    /* 19. return from interruption */
	rfi;;


dump_gregs:
mov r16 = REG_DUMP;;
st8 [r16] = r0;;
add r16 = 8,r16 ;;
st8 [r16] = r1;;
add r16 = 8,r16 ;;
st8 [r16] = r2;;
add r16 = 8,r16 ;;
st8 [r16] = r3;;
add r16 = 8,r16 ;;
st8 [r16] = r4;;
add r16 = 8,r16 ;;
st8 [r16] = r5;;
add r16 = 8,r16 ;;
st8 [r16] = r6;;
add r16 = 8,r16 ;;
st8 [r16] = r7;;
add r16 = 8,r16 ;;
st8 [r16] = r8;;
add r16 = 8,r16 ;;
st8 [r16] = r9;;
add r16 = 8,r16 ;;
st8 [r16] = r10;;
add r16 = 8,r16 ;;
st8 [r16] = r11;;
add r16 = 8,r16 ;;
st8 [r16] = r12;;
add r16 = 8,r16 ;;
st8 [r16] = r13;;
add r16 = 8,r16 ;;
st8 [r16] = r14;;
add r16 = 8,r16 ;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;

bsw.1;;
mov r15 = r16;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r17;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r18;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r19;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r20;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r21;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r22;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r23;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r24;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r25;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r26;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r27;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r28;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r29;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r30;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r31;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;


st8 [r16] = r32;;
add r16 = 8,r16 ;;
st8 [r16] = r33;;
add r16 = 8,r16 ;;
st8 [r16] = r34;;
add r16 = 8,r16 ;;
st8 [r16] = r35;;
add r16 = 8,r16 ;;
st8 [r16] = r36;;
add r16 = 8,r16 ;;
st8 [r16] = r37;;
add r16 = 8,r16 ;;
st8 [r16] = r38;;
add r16 = 8,r16 ;;
st8 [r16] = r39;;
add r16 = 8,r16 ;;
st8 [r16] = r40;;
add r16 = 8,r16 ;;
st8 [r16] = r41;;
add r16 = 8,r16 ;;
st8 [r16] = r42;;
add r16 = 8,r16 ;;
st8 [r16] = r43;;
add r16 = 8,r16 ;;
st8 [r16] = r44;;
add r16 = 8,r16 ;;
st8 [r16] = r45;;
add r16 = 8,r16 ;;
st8 [r16] = r46;;
add r16 = 8,r16 ;;
st8 [r16] = r47;;
add r16 = 8,r16 ;;
st8 [r16] = r48;;
add r16 = 8,r16 ;;
st8 [r16] = r49;;
add r16 = 8,r16 ;;
st8 [r16] = r50;;
add r16 = 8,r16 ;;
st8 [r16] = r51;;
add r16 = 8,r16 ;;
st8 [r16] = r52;;
add r16 = 8,r16 ;;
st8 [r16] = r53;;
add r16 = 8,r16 ;;
st8 [r16] = r54;;
add r16 = 8,r16 ;;
st8 [r16] = r55;;
add r16 = 8,r16 ;;
st8 [r16] = r56;;
add r16 = 8,r16 ;;
st8 [r16] = r57;;
add r16 = 8,r16 ;;
st8 [r16] = r58;;
add r16 = 8,r16 ;;
st8 [r16] = r59;;
add r16 = 8,r16 ;;
st8 [r16] = r60;;
add r16 = 8,r16 ;;
st8 [r16] = r61;;
add r16 = 8,r16 ;;
st8 [r16] = r62;;
add r16 = 8,r16 ;;
st8 [r16] = r63;;
add r16 = 8,r16 ;;



st8 [r16] = r64;;
add r16 = 8,r16 ;;
st8 [r16] = r65;;
add r16 = 8,r16 ;;
st8 [r16] = r66;;
add r16 = 8,r16 ;;
st8 [r16] = r67;;
add r16 = 8,r16 ;;
st8 [r16] = r68;;
add r16 = 8,r16 ;;
st8 [r16] = r69;;
add r16 = 8,r16 ;;
st8 [r16] = r70;;
add r16 = 8,r16 ;;
st8 [r16] = r71;;
add r16 = 8,r16 ;;
st8 [r16] = r72;;
add r16 = 8,r16 ;;
st8 [r16] = r73;;
add r16 = 8,r16 ;;
st8 [r16] = r74;;
add r16 = 8,r16 ;;
st8 [r16] = r75;;
add r16 = 8,r16 ;;
st8 [r16] = r76;;
add r16 = 8,r16 ;;
st8 [r16] = r77;;
add r16 = 8,r16 ;;
st8 [r16] = r78;;
add r16 = 8,r16 ;;
st8 [r16] = r79;;
add r16 = 8,r16 ;;
st8 [r16] = r80;;
add r16 = 8,r16 ;;
st8 [r16] = r81;;
add r16 = 8,r16 ;;
st8 [r16] = r82;;
add r16 = 8,r16 ;;
st8 [r16] = r83;;
add r16 = 8,r16 ;;
st8 [r16] = r84;;
add r16 = 8,r16 ;;
st8 [r16] = r85;;
add r16 = 8,r16 ;;
st8 [r16] = r86;;
add r16 = 8,r16 ;;
st8 [r16] = r87;;
add r16 = 8,r16 ;;
st8 [r16] = r88;;
add r16 = 8,r16 ;;
st8 [r16] = r89;;
add r16 = 8,r16 ;;
st8 [r16] = r90;;
add r16 = 8,r16 ;;
st8 [r16] = r91;;
add r16 = 8,r16 ;;
st8 [r16] = r92;;
add r16 = 8,r16 ;;
st8 [r16] = r93;;
add r16 = 8,r16 ;;
st8 [r16] = r94;;
add r16 = 8,r16 ;;
st8 [r16] = r95;;
add r16 = 8,r16 ;;



st8 [r16] = r96;;
add r16 = 8,r16 ;;
st8 [r16] = r97;;
add r16 = 8,r16 ;;
st8 [r16] = r98;;
add r16 = 8,r16 ;;
st8 [r16] = r99;;
add r16 = 8,r16 ;;
st8 [r16] = r100;;
add r16 = 8,r16 ;;
st8 [r16] = r101;;
add r16 = 8,r16 ;;
st8 [r16] = r102;;
add r16 = 8,r16 ;;
st8 [r16] = r103;;
add r16 = 8,r16 ;;
st8 [r16] = r104;;
add r16 = 8,r16 ;;
st8 [r16] = r105;;
add r16 = 8,r16 ;;
st8 [r16] = r106;;
add r16 = 8,r16 ;;
st8 [r16] = r107;;
add r16 = 8,r16 ;;
st8 [r16] = r108;;
add r16 = 8,r16 ;;
st8 [r16] = r109;;
add r16 = 8,r16 ;;
st8 [r16] = r110;;
add r16 = 8,r16 ;;
st8 [r16] = r111;;
add r16 = 8,r16 ;;
st8 [r16] = r112;;
add r16 = 8,r16 ;;
st8 [r16] = r113;;
add r16 = 8,r16 ;;
st8 [r16] = r114;;
add r16 = 8,r16 ;;
st8 [r16] = r115;;
add r16 = 8,r16 ;;
st8 [r16] = r116;;
add r16 = 8,r16 ;;
st8 [r16] = r117;;
add r16 = 8,r16 ;;
st8 [r16] = r118;;
add r16 = 8,r16 ;;
st8 [r16] = r119;;
add r16 = 8,r16 ;;
st8 [r16] = r120;;
add r16 = 8,r16 ;;
st8 [r16] = r121;;
add r16 = 8,r16 ;;
st8 [r16] = r122;;
add r16 = 8,r16 ;;
st8 [r16] = r123;;
add r16 = 8,r16 ;;
st8 [r16] = r124;;
add r16 = 8,r16 ;;
st8 [r16] = r125;;
add r16 = 8,r16 ;;
st8 [r16] = r126;;
add r16 = 8,r16 ;;
st8 [r16] = r127;;
add r16 = 8,r16 ;;



br.ret.sptk.many b0;;





.macro Handler o h
.org IVT + \o
br \h;;
.endm

.macro Handler2 o 
.org IVT + \o
br.call.sptk.many b0 = dump_gregs;;
mov r16 = \o ;;
bsw.1;;
br universal_handler;;
.endm



.global IVT
.align 32768
IVT:


Handler2 0x0000
Handler2 0x0400
Handler2 0x0800
Handler2 0x0c00
Handler2 0x1000
Handler2 0x1400
Handler2 0x1800
Handler2 0x1c00
Handler2 0x2000
Handler2 0x2400
Handler2 0x2800
Handler 0x2c00 break_instruction
HEAVYWEIGHT_HANDLER 0x3000 external_interrupt	/* For external interrupt, heavyweight handler is used. */
Handler2 0x3400
Handler2 0x3800
Handler2 0x3c00
Handler2 0x4000
Handler2 0x4400
Handler2 0x4800
Handler2 0x4c00

Handler2 0x5000
Handler2 0x5100
Handler2 0x5200
Handler2 0x5300
#Handler 0x5400 general_exception
Handler2 0x5400
Handler2 0x5500
Handler2 0x5600
Handler2 0x5700
Handler2 0x5800
Handler2 0x5900
Handler2 0x5a00
Handler2 0x5b00
Handler2 0x5c00
Handler2 0x5d00
Handler2 0x5e00
Handler2 0x5f00

Handler2 0x6000
Handler2 0x6100
Handler2 0x6200
Handler2 0x6300
Handler2 0x6400
Handler2 0x6500
Handler2 0x6600
Handler2 0x6700
Handler2 0x6800
Handler2 0x6900
Handler2 0x6a00
Handler2 0x6b00
Handler2 0x6c00
Handler2 0x6d00
Handler2 0x6e00
Handler2 0x6f00

Handler2 0x7000
Handler2 0x7100
Handler2 0x7200
Handler2 0x7300
Handler2 0x7400
Handler2 0x7500
Handler2 0x7600
Handler2 0x7700
Handler2 0x7800
Handler2 0x7900
Handler2 0x7a00
Handler2 0x7b00
Handler2 0x7c00
Handler2 0x7d00
Handler2 0x7e00
Handler2 0x7f00








.align 32768
.global REG_DUMP

REG_DUMP:
.space 128*8

