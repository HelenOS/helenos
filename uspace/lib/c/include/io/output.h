/*
 * Copyright (c) 2011 Martin Decky
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

#ifndef LIBC_IO_OUTPUT_H_
#define LIBC_IO_OUTPUT_H_

#include <ipc/output.h>
#include <io/chargrid.h>
#include <io/console.h>

extern errno_t output_yield(async_sess_t *);
extern errno_t output_claim(async_sess_t *);
extern errno_t output_get_dimensions(async_sess_t *, sysarg_t *, sysarg_t *);
extern errno_t output_get_caps(async_sess_t *, console_caps_t *);

extern frontbuf_handle_t output_frontbuf_create(async_sess_t *, chargrid_t *);

extern errno_t output_cursor_update(async_sess_t *, frontbuf_handle_t);
extern errno_t output_set_style(async_sess_t *, console_style_t);

extern errno_t output_update(async_sess_t *, frontbuf_handle_t);
extern errno_t output_damage(async_sess_t *, frontbuf_handle_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t);

#endif

/** @}
 */
