/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#ifndef _LIBC_FIBRIL_H_
#define _LIBC_FIBRIL_H_

#include <time.h>
#include <_bits/errno.h>
#include <_bits/__noreturn.h>
#include <_bits/decls.h>

__HELENOS_DECLS_BEGIN;

typedef struct fibril fibril_t;

typedef struct {
	fibril_t *owned_by;
} fibril_owner_info_t;

typedef fibril_t *fid_t;

#ifndef __cplusplus
/** Fibril-local variable specifier */
#define fibril_local __thread
#endif

extern fid_t fibril_create_generic(errno_t (*)(void *), void *, size_t);
extern fid_t fibril_create(errno_t (*)(void *), void *);
extern void fibril_destroy(fid_t);
extern void fibril_add_ready(fid_t);
extern fid_t fibril_get_id(void);
extern void fibril_yield(void);

extern void fibril_usleep(usec_t);
extern void fibril_sleep(sec_t);

extern void fibril_enable_multithreaded(void);
extern int fibril_test_spawn_runners(int);

extern void fibril_detach(fid_t fid);

extern void fibril_start(fid_t);
extern __noreturn void fibril_exit(long);

__HELENOS_DECLS_END;

#endif

/** @}
 */
