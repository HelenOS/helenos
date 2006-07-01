/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

/** @addtogroup mips32	
 * @{
 */
/** @file
 */

#ifndef __mips32_CONTEXT_H__
#define __mips32_CONTEXT_H__

#include <align.h>
#include <arch/stack.h>

/*
 * Put one item onto the stack to support get_stack_base() and align it up.
 */
#define SP_DELTA	(0 + ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT))


#ifndef __ASM__

#ifndef __mips32_TYPES_H__
# include <arch/types.h>
#endif

/*
 * Only save registers that must be preserved across
 * function calls.
 */
struct context {
	__address sp;
	__address pc;
	
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

	ipl_t ipl;
};

#endif /* __ASM__ */

#endif

/** @}
 */
