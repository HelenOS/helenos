/*
 * Copyright (C) 2005 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ppc32_MACRO_H__
#define __ppc32_MACRO_H__

/*
 * PPC assembler macros
 */
 
/* Condition Register Bit Fields */
#define	cr0	0
#define	cr1	1
#define	cr2	2
#define	cr3	3
#define	cr4	4
#define	cr5	5
#define	cr6	6
#define	cr7	7

/* General Purpose Registers (GPRs) */
#define	r0	0
#define	r1	1
#define	r2	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6
#define	r7	7
#define	r8	8
#define	r9	9
#define	r10	10
#define	r11	11
#define	r12	12
#define	r13	13
#define	r14	14
#define	r15	15
#define	r16	16
#define	r17	17
#define	r18	18
#define	r19	19
#define	r20	20
#define	r21	21
#define	r22	22
#define	r23	23
#define	r24	24
#define	r25	25
#define	r26	26
#define	r27	27
#define	r28	28
#define	r29	29
#define	r30	30
#define	r31	31

/* GPR Aliases */
#define	sp	1

/* Floating Point Registers (FPRs) */
#define	fr0	0
#define	fr1	1
#define	fr2	2
#define	fr3	3
#define	fr4	4
#define	fr5	5
#define	fr6	6
#define	fr7	7
#define	fr8	8
#define	fr9	9
#define	fr10	10
#define	fr11	11
#define	fr12	12
#define	fr13	13
#define	fr14	14
#define	fr15	15
#define	fr16	16
#define	fr17	17
#define	fr18	18
#define	fr19	19
#define	fr20	20
#define	fr21	21
#define	fr22	22
#define	fr23	23
#define	fr24	24
#define	fr25	25
#define	fr26	26
#define	fr27	27
#define	fr28	28
#define	fr29	29
#define	fr30	30
#define	fr31	31

#define	vr0	0
#define	vr1	1
#define	vr2	2
#define	vr3	3
#define	vr4	4
#define	vr5	5
#define	vr6	6
#define	vr7	7
#define	vr8	8
#define	vr9	9
#define	vr10	10
#define	vr11	11
#define	vr12	12
#define	vr13	13
#define	vr14	14
#define	vr15	15
#define	vr16	16
#define	vr17	17
#define	vr18	18
#define	vr19	19
#define	vr20	20
#define	vr21	21
#define	vr22	22
#define	vr23	23
#define	vr24	24
#define	vr25	25
#define	vr26	26
#define	vr27	27
#define	vr28	28
#define	vr29	29
#define	vr30	30
#define	vr31	31

#define	evr0	0
#define	evr1	1
#define	evr2	2
#define	evr3	3
#define	evr4	4
#define	evr5	5
#define	evr6	6
#define	evr7	7
#define	evr8	8
#define	evr9	9
#define	evr10	10
#define	evr11	11
#define	evr12	12
#define	evr13	13
#define	evr14	14
#define	evr15	15
#define	evr16	16
#define	evr17	17
#define	evr18	18
#define	evr19	19
#define	evr20	20
#define	evr21	21
#define	evr22	22
#define	evr23	23
#define	evr24	24
#define	evr25	25
#define	evr26	26
#define	evr27	27
#define	evr28	28
#define	evr29	29
#define	evr30	30
#define	evr31	31

/* Special Purpose Registers (SPRs) */
#define	xer	1
#define lr	8
#define ctr	9
#define	dec	22
#define	srr0	26
#define srr1	27
#define	sprg0	272
#define	sprg1	273
#define	sprg2	274
#define	sprg3	275
#define	prv	287

.macro REGISTERS_STORE r
	stw r0, 0(\r)
	stw r1, 4(\r)
	stw r2, 8(\r)
	stw r3, 12(\r)
	stw r4, 16(\r)
	stw r5, 20(\r)
	stw r6, 24(\r)
	stw r7, 28(\r)
	stw r8, 32(\r)
	stw r9, 36(\r)
	stw r10, 40(\r)
	stw r11, 44(\r)
	stw r12, 48(\r)
	stw r13, 52(\r)
	stw r14, 56(\r)
	stw r15, 60(\r)
	stw r16, 64(\r)
	stw r17, 68(\r)
	stw r18, 72(\r)
	stw r19, 76(\r)
	stw r20, 80(\r)
	stw r21, 84(\r)
	stw r22, 88(\r)
	stw r23, 92(\r)
	stw r24, 96(\r)
	stw r25, 100(\r)
	stw r26, 104(\r)
	stw r27, 108(\r)
	stw r28, 112(\r)
	stw r29, 116(\r)
	stw r30, 120(\r)
	stw r31, 124(\r)
.endm

.macro REGISTERS_LOAD r
	lwz r0, 0(\r)
	lwz r1, 4(\r)
	lwz r2, 8(\r)  
	lwz r3, 12(\r)
	lwz r4, 16(\r) 
	lwz r5, 20(\r)
	lwz r6, 24(\r)
	lwz r7, 28(\r)
	lwz r8, 32(\r)
	lwz r9, 36(\r)
	lwz r10, 40(\r)
	lwz r11, 44(\r)
	lwz r12, 48(\r)
	lwz r13, 52(\r)
	lwz r14, 56(\r)
	lwz r15, 60(\r)
	lwz r16, 64(\r)
	lwz r17, 68(\r)
	lwz r18, 72(\r)
	lwz r19, 76(\r)
	lwz r20, 80(\r)
	lwz r21, 84(\r)
	lwz r22, 88(\r)
	lwz r23, 92(\r)
	lwz r24, 96(\r)
	lwz r25, 100(\r)
	lwz r26, 104(\r)
	lwz r27, 108(\r)
	lwz r28, 112(\r)
	lwz r29, 116(\r)
	lwz r30, 120(\r)
	lwz r31, 124(\r)
.endm

#endif
