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

#ifndef KERN_mips32_CONTEXT_OFFSET_H_
#define KERN_mips32_CONTEXT_OFFSET_H_

#define OFFSET_SP      0x0
#define OFFSET_PC      0x4
#define OFFSET_S0      0x8
#define OFFSET_S1      0xc
#define OFFSET_S2      0x10
#define OFFSET_S3      0x14
#define OFFSET_S4      0x18
#define OFFSET_S5      0x1c
#define OFFSET_S6      0x20
#define OFFSET_S7      0x24
#define OFFSET_S8      0x28
#define OFFSET_GP      0x2c

#ifdef KERNEL
# define OFFSET_IPL     0x30
#else
# define OFFSET_TLS     0x30

# define OFFSET_F20     0x34
# define OFFSET_F21     0x38
# define OFFSET_F22     0x3c
# define OFFSET_F23     0x40
# define OFFSET_F24     0x44
# define OFFSET_F25     0x48
# define OFFSET_F26     0x4c
# define OFFSET_F27     0x50
# define OFFSET_F28     0x54
# define OFFSET_F29     0x58
# define OFFSET_F30     0x5c
#endif /* KERNEL */

#ifdef __ASM__

#ifdef KERNEL

#include <arch/asm/regname.h>

#else /* KERNEL */

#include <libarch/regname.h>

#endif /* KERNEL */

/* ctx: address of the structure with saved context */
.macro CONTEXT_SAVE_ARCH_CORE ctx:req
	sw $s0, OFFSET_S0(\ctx)
	sw $s1, OFFSET_S1(\ctx)
	sw $s2, OFFSET_S2(\ctx)
	sw $s3, OFFSET_S3(\ctx)
	sw $s4, OFFSET_S4(\ctx)
	sw $s5, OFFSET_S5(\ctx)
	sw $s6, OFFSET_S6(\ctx)
	sw $s7, OFFSET_S7(\ctx)
	sw $s8, OFFSET_S8(\ctx)
	sw $gp, OFFSET_GP(\ctx)
	
#ifndef KERNEL
	sw $k1, OFFSET_TLS(\ctx)
	
#ifdef CONFIG_FPU
	mfc1 $t0, $20
	sw $t0, OFFSET_F20(\ctx)
	
	mfc1 $t0, $21
	sw $t0, OFFSET_F21(\ctx)
	
	mfc1 $t0, $22
	sw $t0, OFFSET_F22(\ctx)
	
	mfc1 $t0, $23
	sw $t0, OFFSET_F23(\ctx)
	
	mfc1 $t0, $24
	sw $t0, OFFSET_F24(\ctx)
	
	mfc1 $t0, $25
	sw $t0, OFFSET_F25(\ctx)
	
	mfc1 $t0, $26
	sw $t0, OFFSET_F26(\ctx)
	
	mfc1 $t0, $27
	sw $t0, OFFSET_F27(\ctx)
	
	mfc1 $t0, $28
	sw $t0, OFFSET_F28(\ctx)
	
	mfc1 $t0, $29
	sw $t0, OFFSET_F29(\ctx)
	
	mfc1 $t0, $30
	sw $t0, OFFSET_F30(\ctx)
#endif /* CONFIG_FPU */
#endif /* KERNEL */
	
	sw $ra, OFFSET_PC(\ctx)
	sw $sp, OFFSET_SP(\ctx)
.endm

/* ctx: address of the structure with saved context */
.macro CONTEXT_RESTORE_ARCH_CORE ctx:req
	lw $s0, OFFSET_S0(\ctx)
	lw $s1, OFFSET_S1(\ctx)
	lw $s2, OFFSET_S2(\ctx)
	lw $s3, OFFSET_S3(\ctx)
	lw $s4, OFFSET_S4(\ctx)
	lw $s5, OFFSET_S5(\ctx)
	lw $s6, OFFSET_S6(\ctx)
	lw $s7, OFFSET_S7(\ctx)
	lw $s8, OFFSET_S8(\ctx)
	lw $gp, OFFSET_GP(\ctx)
#ifndef KERNEL
	lw $k1, OFFSET_TLS(\ctx)
	
#ifdef CONFIG_FPU
	lw $t0, OFFSET_F20(\ctx)
	mtc1 $t0, $20
	
	lw $t0, OFFSET_F21(\ctx)
	mtc1 $t0, $21
	
	lw $t0, OFFSET_F22(\ctx)
	mtc1 $t0, $22
	
	lw $t0, OFFSET_F23(\ctx)
	mtc1 $t0, $23
	
	lw $t0, OFFSET_F24(\ctx)
	mtc1 $t0, $24
	
	lw $t0, OFFSET_F25(\ctx)
	mtc1 $t0, $25
	
	lw $t0, OFFSET_F26(\ctx)
	mtc1 $t0, $26
	
	lw $t0, OFFSET_F27(\ctx)
	mtc1 $t0, $27
	
	lw $t0, OFFSET_F28(\ctx)
	mtc1 $t0, $28
	
	lw $t0, OFFSET_F29(\ctx)
	mtc1 $t0, $29
	
	lw $t0, OFFSET_F30(\ctx)
	mtc1 $t0, $30
#endif /* CONFIG_FPU */
#endif /* KERNEL */
	
	lw $ra, OFFSET_PC(\ctx)
	lw $sp, OFFSET_SP(\ctx)
.endm

#endif /* __ASM__ */

#endif
