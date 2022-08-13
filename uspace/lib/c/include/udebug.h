/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_UDEBUG_H_
#define _LIBC_UDEBUG_H_

#include <abi/udebug.h>
#include <stddef.h>
#include <stdint.h>
#include <async.h>

typedef sysarg_t thash_t;

extern errno_t udebug_begin(async_sess_t *);
extern errno_t udebug_end(async_sess_t *);
extern errno_t udebug_set_evmask(async_sess_t *, udebug_evmask_t);
extern errno_t udebug_thread_read(async_sess_t *, void *, size_t, size_t *,
    size_t *);
extern errno_t udebug_name_read(async_sess_t *, void *, size_t, size_t *,
    size_t *);
extern errno_t udebug_areas_read(async_sess_t *, void *, size_t, size_t *,
    size_t *);
extern errno_t udebug_mem_read(async_sess_t *, void *, uintptr_t, size_t);
extern errno_t udebug_args_read(async_sess_t *, thash_t, sysarg_t *);
extern errno_t udebug_regs_read(async_sess_t *, thash_t, void *);
extern errno_t udebug_go(async_sess_t *, thash_t, udebug_event_t *, sysarg_t *,
    sysarg_t *);
extern errno_t udebug_stop(async_sess_t *, thash_t);

#endif

/** @}
 */
