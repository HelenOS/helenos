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
