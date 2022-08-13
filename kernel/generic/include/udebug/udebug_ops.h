/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_UDEBUG_OPS_H_
#define KERN_UDEBUG_OPS_H_

#include <ipc/ipc.h>
#include <proc/thread.h>
#include <stdbool.h>
#include <stddef.h>

errno_t udebug_begin(call_t *call, bool *active);
errno_t udebug_end(void);
errno_t udebug_set_evmask(udebug_evmask_t mask);

errno_t udebug_go(thread_t *t, call_t *call);
errno_t udebug_stop(thread_t *t, call_t *call);

errno_t udebug_thread_read(void **buffer, size_t buf_size, size_t *stored,
    size_t *needed);
errno_t udebug_name_read(char **data, size_t *data_size);
errno_t udebug_args_read(thread_t *t, void **buffer);

errno_t udebug_regs_read(thread_t *t, void **buffer);

errno_t udebug_mem_read(uspace_addr_t uspace_addr, size_t n, void **buffer);

#endif

/** @}
 */
