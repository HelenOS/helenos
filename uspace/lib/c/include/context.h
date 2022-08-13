/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_CONTEXT_H_
#define _LIBC_CONTEXT_H_

#include <_bits/size_t.h>
#include <libarch/fibril_context.h>

typedef __context_t context_t;

/* Context initialization data. */
typedef struct {
	void (*fn)(void);
	void *stack_base;
	size_t stack_size;
	void *tls;
} context_create_t;

extern void context_swap(context_t *self, context_t *other);
extern void context_create(context_t *context, const context_create_t *arg);
extern uintptr_t context_get_fp(context_t *ctx);
extern uintptr_t context_get_pc(context_t *ctx);

#endif
