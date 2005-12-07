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

#ifndef __sparc64_STACK_H__
# include <arch/stack.h>
#endif

#ifndef __sparc64_TYPES_H__
# include <arch/types.h>
#endif

#ifndef __ALIGN_H__
# include <align.h>
#endif

#define SP_DELTA	STACK_WINDOW_SAVE_AREA_SIZE

#ifdef context_set
#undef context_set
#endif

#define context_set(c, _pc, stack, size)								\
        (c)->pc = ((__address) _pc) - 8;								\
        (c)->sp = ((__address) stack) + ALIGN_UP((size), STACK_ALIGNMENT) - (STACK_BIAS + SP_DELTA);	\
	(c)->fp = -STACK_BIAS
	

/*
 * Only save registers that must be preserved across
 * function calls and that are not saved in caller's
 * register window.
 */
struct context {
	__u64 o1;
	__u64 o2;
	__u64 o3;
	__u64 o4;
	__u64 o5;
	__address sp;		/* %o6 */
	__address pc;		/* %o7 */
	__address fp;
	ipl_t ipl;
};

#endif
