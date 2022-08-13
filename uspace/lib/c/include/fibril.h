/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
