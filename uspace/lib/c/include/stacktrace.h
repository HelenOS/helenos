/*
 * Copyright (c) 2009 Jakub Jermar
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_STACKTRACE_H_
#define LIBC_STACKTRACE_H_

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	errno_t (*read_uintptr)(void *, uintptr_t, uintptr_t *);
} stacktrace_ops_t;

typedef struct {
	void *op_arg;
	stacktrace_ops_t *ops;
} stacktrace_t;

extern void stacktrace_print(void);
extern void stacktrace_print_fp_pc(uintptr_t, uintptr_t);
extern void stacktrace_print_generic(stacktrace_ops_t *, void *, uintptr_t,
    uintptr_t);

/*
 * The following interface is to be implemented by each architecture.
 */
extern bool stacktrace_fp_valid(stacktrace_t *, uintptr_t);
extern errno_t stacktrace_fp_prev(stacktrace_t *, uintptr_t, uintptr_t *);
extern errno_t stacktrace_ra_get(stacktrace_t *, uintptr_t, uintptr_t *);

extern void stacktrace_prepare(void);
extern uintptr_t stacktrace_fp_get(void);
extern uintptr_t stacktrace_pc_get(void);

#endif

/** @}
 */
