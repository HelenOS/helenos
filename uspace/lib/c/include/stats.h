/*
 * SPDX-FileCopyrightText: 2010 Stanislav Kozina
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_STATS_H_
#define _LIBC_STATS_H_

#include <task.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <abi/sysinfo.h>

#define LOAD_UNIT  65536

extern stats_cpu_t *stats_get_cpus(size_t *);
extern stats_physmem_t *stats_get_physmem(void);
extern load_t *stats_get_load(size_t *);

extern stats_task_t *stats_get_tasks(size_t *);
extern stats_task_t *stats_get_task(task_id_t);

extern stats_thread_t *stats_get_threads(size_t *);
extern stats_ipcc_t *stats_get_ipccs(size_t *);

extern stats_exc_t *stats_get_exceptions(size_t *);
extern stats_exc_t *stats_get_exception(unsigned int);

extern void stats_print_load_fragment(load_t, unsigned int);
extern const char *thread_get_state(state_t);

#endif

/** @}
 */
