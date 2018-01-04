/*
 * Copyright (c) 2011 Petr Koupy
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

#ifndef LIBC_IO_VISUALIZER_H_
#define LIBC_IO_VISUALIZER_H_

#include <async.h>
#include <io/mode.h>

extern errno_t visualizer_claim(async_sess_t *, sysarg_t);
extern errno_t visualizer_yield(async_sess_t *);

extern errno_t visualizer_enumerate_modes(async_sess_t *, vslmode_t *, sysarg_t);
extern errno_t visualizer_get_default_mode(async_sess_t *, vslmode_t *);
extern errno_t visualizer_get_current_mode(async_sess_t *, vslmode_t *);
extern errno_t visualizer_get_mode(async_sess_t *, vslmode_t *, sysarg_t);
extern errno_t visualizer_set_mode(async_sess_t *, sysarg_t, sysarg_t, void *);

extern errno_t visualizer_update_damaged_region(async_sess_t *,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t);

extern errno_t visualizer_suspend(async_sess_t *);
extern errno_t visualizer_wakeup(async_sess_t *);

#endif

/** @}
 */
