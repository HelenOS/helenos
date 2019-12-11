/*
 * Copyright (c) 2010 Stanislav Kozina
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
