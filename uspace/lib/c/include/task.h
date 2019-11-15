/*
 * Copyright (c) 2006 Jakub Jermar
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

#ifndef _LIBC_TASK_H_
#define _LIBC_TASK_H_

#include <async.h>
#include <stdint.h>
#include <stdarg.h>
#include <abi/proc/task.h>
#include <async.h>
#include <types/task.h>

typedef struct {
	ipc_call_t result;
	aid_t aid;
} task_wait_t;

struct _TASK;
typedef struct _TASK task_t;

extern task_id_t task_get_id(void);
extern errno_t task_set_name(const char *);
extern errno_t task_kill(task_id_t);

extern errno_t task_spawnv(task_id_t *, task_wait_t *, const char *path,
    const char *const []);
extern errno_t task_spawnv_debug(task_id_t *, task_wait_t *, const char *path,
    const char *const [], async_sess_t **);
extern errno_t task_spawnvf(task_id_t *, task_wait_t *, const char *path,
    const char *const [], int, int, int);
extern errno_t task_spawnvf_debug(task_id_t *, task_wait_t *, const char *path,
    const char *const [], int, int, int, async_sess_t **);
extern errno_t task_spawn(task_id_t *, task_wait_t *, const char *path, int,
    va_list ap);
extern errno_t task_spawnl(task_id_t *, task_wait_t *, const char *path, ...)
    __attribute__((sentinel));

extern errno_t task_setup_wait(task_id_t, task_wait_t *);
extern void task_cancel_wait(task_wait_t *);
extern errno_t task_wait(task_wait_t *, task_exit_t *, int *);
extern errno_t task_wait_task_id(task_id_t, task_exit_t *, int *);
extern errno_t task_retval(int);

#endif

/** @}
 */
