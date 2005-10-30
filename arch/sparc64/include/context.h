/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __sparc64_CONTEXT_H__
#define __sparc64_CONTEXT_H__

#ifndef __sparc64_TYPES_H__
# include <arch/types.h>
#endif

#ifndef __ALIGN_H__
# include <align.h>
#endif

/** According to SPARC Compliance Definition, every stack frame is 16-byte aligned. */
#define STACK_ALIGNMENT			16

#define STACK_ITEM_SIZE			sizeof(__u64)

/**
 * 16-extended-word save area for %i[0-7] and %l[0-7] registers.
 */
#define SAVE_AREA	(16*STACK_ITEM_SIZE)
#define SP_DELTA	SAVE_AREA

/**
 * By convention, the actual top of the stack is %sp + BIAS.
 */
#define BIAS		2047

#ifdef context_set
#undef context_set
#endif

#define context_set(c, _pc, stack, size)                                                                \
        (c)->pc = ((__address) _pc) - 8;                                                                \
        (c)->sp = ((__address) stack) + ALIGN((size), STACK_ALIGNMENT) - (BIAS + SP_DELTA)

/*
 * Only save registers that must be preserved across
 * function calls.
 */
struct context {
	__u64 l0;
	__u64 l1;
	__u64 l2;
	__u64 l3;
	__u64 l4;
	__u64 l5;
	__u64 l6;
	__u64 l7;
	__u64 i1;
	__u64 i2;
	__u64 i3;
	__u64 i4;
	__u64 i5;
	__address sp;		/* %i6 */
	__address pc;		/* %i7 */
	ipl_t ipl;
};

#endif
