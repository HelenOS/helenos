/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
