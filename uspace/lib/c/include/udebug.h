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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_UDEBUG_H_
#define LIBC_UDEBUG_H_

#include <kernel/udebug/udebug.h>
#include <sys/types.h>

typedef sysarg_t thash_t;

int udebug_begin(int);
int udebug_end(int);
int udebug_set_evmask(int, udebug_evmask_t);
int udebug_thread_read(int, void *, size_t , size_t *, size_t *);
int udebug_name_read(int, void *, size_t, size_t *, size_t *);
int udebug_areas_read(int, void *, size_t, size_t *, size_t *);
int udebug_mem_read(int, void *, uintptr_t, size_t);
int udebug_args_read(int, thash_t, sysarg_t *);
int udebug_regs_read(int, thash_t, void *);
int udebug_go(int, thash_t, udebug_event_t *, sysarg_t *, sysarg_t *);
int udebug_stop(int, thash_t);

#endif

/** @}
 */
