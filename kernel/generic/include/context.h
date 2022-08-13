/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_CONTEXT_H_
#define KERN_CONTEXT_H_

#include <trace.h>
#include <arch/context.h>

#define context_set_generic(ctx, _pc, stack, size) \
	do { \
		(ctx)->pc = (uintptr_t) (_pc); \
		(ctx)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
	} while (0)

extern int context_save_arch(context_t *ctx) __attribute__((returns_twice));
extern void context_restore_arch(context_t *ctx) __attribute__((noreturn));

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
 * @param ctx Context structure.
 *
 * @return context_save() returns 1, context_restore() returns 0.
 *
 */
#define context_save(ctx)  context_save_arch(ctx)

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
_NO_TRACE static inline void context_restore(context_t *ctx)
{
	context_restore_arch(ctx);
}

#endif

/** @}
 */
