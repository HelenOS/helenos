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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_CONTEXT_H_
#define KERN_CONTEXT_H_

#include <panic.h>
#include <trace.h>
#include <arch/context.h>
#include <arch/faddr.h>

#define context_set_generic(ctx, _pc, stack, size) \
	do { \
		(ctx)->pc = (uintptr_t) (_pc); \
		(ctx)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
	} while (0)

extern int context_save_arch(context_t *ctx) __attribute__((returns_twice));
extern void context_restore_arch(context_t *ctx) __attribute__((noreturn));

/** Restore register context.
 *
 * Restore a previously saved register context (including stack pointer) from
 * a context structure.
 *
 * Note that this function does not normally return.  Instead, it returns to the
 * same address as the corresponding call to context_save(), the only difference
 * being return value.
 *
 * @param ctx Context structure.
 *
 */
_NO_TRACE __attribute__((noreturn))
    static inline void context_restore(context_t *ctx)
{
	context_restore_arch(ctx);
}

/**
 * Saves current context to the variable pointed to by `self`,
 * and restores the context denoted by `other`.
 *
 * When the `self` context is later restored by another call to
 * `context_swap()`, the control flow behaves as if the earlier call to
 * `context_swap()` just returned.
 */
_NO_TRACE static inline void context_swap(context_t *self, context_t *other)
{
	if (context_save_arch(self))
		context_restore_arch(other);
}

_NO_TRACE static inline void context_create(context_t *context,
    void (*fn)(void), void *stack_base, size_t stack_size)
{
	*context = (context_t) { 0 };
	context_set(context, FADDR(fn), stack_base, stack_size);
}

__attribute__((noreturn)) static inline void context_replace(void (*fn)(void),
    void *stack_base, size_t stack_size)
{
	context_t ctx;
	context_create(&ctx, fn, stack_base, stack_size);
	context_restore(&ctx);
}

#endif

/** @}
 */
