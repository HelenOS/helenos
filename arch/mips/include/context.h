/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __mips_CONTEXT_H__
#define __mips_CONTEXT_H__

#define STACK_ITEM_SIZE	4

/* These are offsets into the register dump saved
 * on exception entry
 */
#define EOFFSET_AT 0
#define EOFFSET_V0 4
#define EOFFSET_V1 8
#define EOFFSET_A0 12
#define EOFFSET_A1 16
#define EOFFSET_A2 20
#define EOFFSET_A3 24
#define EOFFSET_A4 28
#define EOFFSET_T1 32
#define EOFFSET_T2 36
#define EOFFSET_T3 40
#define EOFFSET_T4 44
#define EOFFSET_T5 48
#define EOFFSET_T6 52
#define EOFFSET_T7 56
#define EOFFSET_T8 60
#define EOFFSET_T9 64
#define EOFFSET_S0 68
#define EOFFSET_S1 72
#define EOFFSET_S2 76
#define EOFFSET_S3 80
#define EOFFSET_S4 84
#define EOFFSET_S5 88
#define EOFFSET_S6 92
#define EOFFSET_S7 96
#define EOFFSET_S8 100
#define EOFFSET_GP 104
#define EOFFSET_RA 108
#define EOFFSET_LO 112
#define EOFFSET_HI 116

#define REGISTER_SPACE 120

/*
 * Put one item onto the stack to support get_stack_base().
 */
#define SP_DELTA	(0+STACK_ITEM_SIZE)


#ifndef __ASM__

#ifndef __mips_TYPES_H_
# include <arch/types.h>
#endif

/*
 * Only save registers that must be preserved across
 * function calls.
 */
struct context {
	__u32 sp;
	__u32 pc;
	
	__u32 s0;
	__u32 s1;
	__u32 s2;
	__u32 s3;
	__u32 s4;
	__u32 s5;
	__u32 s6;
	__u32 s7;
	__u32 s8;
	__u32 gp;

	__u32 pri;
};

#endif /* __ASM__ */

#endif
