/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_OUTPUT_H_
#define _LIBC_IO_OUTPUT_H_

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
