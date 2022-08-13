/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <context.h>
#include <setjmp.h>
#include <tls.h>
#include <libarch/fibril.h>
#include <libarch/faddr.h>

/**
 * Saves current context to the variable pointed to by `self`,
 * and restores the context denoted by `other`.
 *
 * When the `self` context is later restored by another call to
 * `context_swap()`, the control flow behaves as if the earlier call to
 * `context_swap()` just returned.
 */
void context_swap(context_t *self, context_t *other)
{
	if (!__context_save(self))
		__context_restore(other, 1);
}

void context_create(context_t *context, const context_create_t *arg)
{
	__context_save(context);
	context_set(context, FADDR(arg->fn), arg->stack_base,
	    arg->stack_size, arg->tls);
}

uintptr_t context_get_pc(context_t *ctx)
{
	// This is a simple wrapper for now, and exists to allow
	// potential future implementation of context_swap to omit
	// program counter from the context structure (e.g. if it's
	// stored on the stack).
	return ctx->pc;
}

uintptr_t context_get_fp(context_t *ctx)
{
	return _context_get_fp(ctx);
}
