/*
 * Copyright (c) 2005 Martin Decky
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

#ifndef KERN_mips64_CONTEXT_OFFSET_H_
#define KERN_mips64_CONTEXT_OFFSET_H_

#define OFFSET_SP       0x00
#define OFFSET_PC       0x08
#define OFFSET_S0       0x10
#define OFFSET_S1       0x18
#define OFFSET_S2       0x20
#define OFFSET_S3       0x28
#define OFFSET_S4       0x30
#define OFFSET_S5       0x38
#define OFFSET_S6       0x40
#define OFFSET_S7       0x48
#define OFFSET_S8       0x50
#define OFFSET_GP       0x58

#ifdef KERNEL
	#define OFFSET_IPL  0x60
#else
	#define OFFSET_TLS  0x60
	
	#define OFFSET_F20  0x68
	#define OFFSET_F21  0x70
	#define OFFSET_F22  0x78
	#define OFFSET_F23  0x80
	#define OFFSET_F24  0x88
	#define OFFSET_F25  0x90
	#define OFFSET_F26  0x98
	#define OFFSET_F27  0xa0
	#define OFFSET_F28  0xa8
	#define OFFSET_F29  0xb0
	#define OFFSET_F30  0xb8
#endif /* KERNEL */

#ifdef __ASM__

#ifdef KERNEL

#include <arch/asm/regname.h>

#else /* KERNEL */

#include <libarch/regname.h>

#endif /* KERNEL */

/* ctx: address of the structure with saved context */
.macro CONTEXT_SAVE_ARCH_CORE ctx:req
	sd $s0, OFFSET_S0(\ctx)
	sd $s1, OFFSET_S1(\ctx)
	sd $s2, OFFSET_S2(\ctx)
	sd $s3, OFFSET_S3(\ctx)
	sd $s4, OFFSET_S4(\ctx)
	sd $s5, OFFSET_S5(\ctx)
	sd $s6, OFFSET_S6(\ctx)
	sd $s7, OFFSET_S7(\ctx)
	sd $s8, OFFSET_S8(\ctx)
	sd $gp, OFFSET_GP(\ctx)
	
#ifndef KERNEL
	sd $k1, OFFSET_TLS(\ctx)
	
#ifdef CONFIG_FPU
	dmfc1 $t0, $20
	sd $t0, OFFSET_F20(\ctx)
	
	dmfc1 $t0,$21
	sd $t0, OFFSET_F21(\ctx)
	
	dmfc1 $t0,$22
	sd $t0, OFFSET_F22(\ctx)
	
	dmfc1 $t0,$23
	sd $t0, OFFSET_F23(\ctx)
	
	dmfc1 $t0,$24
	sd $t0, OFFSET_F24(\ctx)
	
	dmfc1 $t0,$25
	sd $t0, OFFSET_F25(\ctx)
	
	dmfc1 $t0,$26
	sd $t0, OFFSET_F26(\ctx)
	
	dmfc1 $t0,$27
	sd $t0, OFFSET_F27(\ctx)
	
	dmfc1 $t0,$28
	sd $t0, OFFSET_F28(\ctx)
	
	dmfc1 $t0,$29
	sd $t0, OFFSET_F29(\ctx)
	
	dmfc1 $t0,$30
	sd $t0, OFFSET_F30(\ctx)
#endif /* CONFIG_FPU */
#endif /* KERNEL */
	
	sd $ra, OFFSET_PC(\ctx)
	sd $sp, OFFSET_SP(\ctx)
.endm

/* ctx: address of the structure with saved context */
.macro CONTEXT_RESTORE_ARCH_CORE ctx:req
	ld $s0, OFFSET_S0(\ctx)
	ld $s1, OFFSET_S1(\ctx)
	ld $s2, OFFSET_S2(\ctx)
	ld $s3, OFFSET_S3(\ctx)
	ld $s4, OFFSET_S4(\ctx)
	ld $s5, OFFSET_S5(\ctx)
	ld $s6, OFFSET_S6(\ctx)
	ld $s7, OFFSET_S7(\ctx)
	ld $s8, OFFSET_S8(\ctx)
	ld $gp, OFFSET_GP(\ctx)
	
#ifndef KERNEL
	ld $k1, OFFSET_TLS(\ctx)
	
#ifdef CONFIG_FPU
	ld $t0, OFFSET_F20(\ctx)
	dmtc1 $t0,$20
	
	ld $t0, OFFSET_F21(\ctx)
	dmtc1 $t0,$21
	
	ld $t0, OFFSET_F22(\ctx)
	dmtc1 $t0,$22
	
	ld $t0, OFFSET_F23(\ctx)
	dmtc1 $t0,$23
	
	ld $t0, OFFSET_F24(\ctx)
	dmtc1 $t0,$24
	
	ld $t0, OFFSET_F25(\ctx)
	dmtc1 $t0,$25
	
	ld $t0, OFFSET_F26(\ctx)
	dmtc1 $t0,$26
	
	ld $t0, OFFSET_F27(\ctx)
	dmtc1 $t0,$27
	
	ld $t0, OFFSET_F28(\ctx)
	dmtc1 $t0,$28
	
	ld $t0, OFFSET_F29(\ctx)
	dmtc1 $t0,$29
	
	ld $t0, OFFSET_F30(\ctx)
	dmtc1 $t0,$30
#endif /* CONFIG_FPU */
#endif /* KERNEL */
	
	ld $ra, OFFSET_PC(\ctx)
	ld $sp, OFFSET_SP(\ctx)
.endm

#endif /* __ASM__ */

#endif
