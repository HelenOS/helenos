/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#ifndef _LIBC_SETJMP_H_
#define _LIBC_SETJMP_H_

#include <libarch/fibril_context.h>
#include <_bits/__noreturn.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

typedef __context_t jmp_buf[1];

extern int __context_save(__context_t *) __attribute__((returns_twice));
extern __noreturn void __context_restore(__context_t *, int);

extern __noreturn void longjmp(jmp_buf, int);

__C_DECLS_END;

#define setjmp __context_save

#endif

/** @}
 */
