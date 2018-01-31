/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup generic
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

errno_t udebug_mem_read(sysarg_t uspace_addr, size_t n, void **buffer);

#endif

/** @}
 */
