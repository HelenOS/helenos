/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief Program loader interface.
 */

#ifndef _LIBC_LOADER_H_
#define _LIBC_LOADER_H_

#include <abi/proc/task.h>

/** Forward declararion */
struct loader;
typedef struct loader loader_t;

extern errno_t loader_spawn(const char *);
extern loader_t *loader_connect(errno_t *);
extern errno_t loader_get_task_id(loader_t *, task_id_t *);
extern errno_t loader_set_cwd(loader_t *);
extern errno_t loader_set_program(loader_t *, const char *, int);
extern errno_t loader_set_program_path(loader_t *, const char *);
extern errno_t loader_set_args(loader_t *, const char *const[]);
extern errno_t loader_add_inbox(loader_t *, const char *, int);
extern errno_t loader_load_program(loader_t *);
extern errno_t loader_run(loader_t *);
extern void loader_run_nowait(loader_t *);
extern void loader_abort(loader_t *);

#endif

/**
 * @}
 */
