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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <arch/types.h>
#include <typedefs.h>
#include <arch/context.h>


#ifndef context_set
#define context_set(c, _pc, stack, size) 	\
	(c)->pc = (uintptr_t) (_pc);		\
	(c)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA;
#endif /* context_set */

extern int context_save_arch(context_t *c);
extern void context_restore_arch(context_t *c) __attribute__ ((noreturn));

/** Save register context.
 *
 * Save current register context (including stack pointers)
 * to context structure.
 *
 * Note that call to context_restore() will return at the same
 * address as the corresponding call to context_save().
 *
 * This MUST be a macro, gcc -O0 does not inline functions even
 * if they are marked inline and context_save_arch must be called
 * from level <= that when context_restore is called.
 *
 * @param c Context structure.
 *
 * @return context_save() returns 1, context_restore() returns 0.
 */
#define context_save(c)   context_save_arch(c)

/** Restore register context.
 *
 * Restore previously saved register context (including stack pointers)
 * from context structure.
 *
 * Note that this function does not normally return.
 * Instead, it returns at the same address as the
 * corresponding call to context_save(), the only
 * difference being return value.
 *
 * Note that content of any local variable defined by
 * the caller of context_save() is undefined after
 * context_restore().
 *
 * @param c Context structure.
 */
static inline void context_restore(context_t *c)
{
	context_restore_arch(c);
}

#endif

/** @}
 */
