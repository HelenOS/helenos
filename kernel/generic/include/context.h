/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef KERN_CONTEXT_H_
#define KERN_CONTEXT_H_

#include <arch/types.h>
#include <arch/context.h>


#ifndef context_set
#define context_set(c, _pc, stack, size) 	\
	(c)->pc = (uintptr_t) (_pc);		\
	(c)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA;
#endif /* context_set */

extern int context_save_arch(context_t *c) __attribute__ ((returns_twice));
extern void context_restore_arch(context_t *c) __attribute__ ((noreturn));

/** Save register context.
 *
 * Save the current register context (including stack pointer) to a context
 * structure. A subsequent call to context_restore() will return to the same
 * address as the corresponding call to context_save().
 *
 * Note that context_save_arch() must reuse the stack frame of the function
 * which called context_save(). We guarantee this by:
 *
 *   a) implementing context_save_arch() in assembly so that it does not create
 *      its own stack frame, and by
 *   b) defining context_save() as a macro because the inline keyword is just a
 *      hint for the compiler, not a real constraint; the application of a macro
 *      will definitely not create a stack frame either.
 *
 * To imagine what could happen if there were some extra stack frames created
 * either by context_save() or context_save_arch(), we need to realize that the
 * sp saved in the contex_t structure points to the current stack frame as it
 * existed when context_save_arch() was executing. After the return from
 * context_save_arch() and context_save(), any extra stack frames created by
 * these functions will be destroyed and their contents sooner or later
 * overwritten by functions called next. Any attempt to restore to a context
 * saved like that would therefore lead to a disaster.
 *
 * @param c		Context structure.
 *
 * @return		context_save() returns 1, context_restore() returns 0.
 */
#define context_save(c)   context_save_arch(c)

/** Restore register context.
 *
 * Restore a previously saved register context (including stack pointer) from
 * a context structure.
 *
 * Note that this function does not normally return.  Instead, it returns to the
 * same address as the corresponding call to context_save(), the only difference
 * being return value.
 *
 * @param c		Context structure.
 */
static inline void context_restore(context_t *c)
{
	context_restore_arch(c);
}

#endif

/** @}
 */
