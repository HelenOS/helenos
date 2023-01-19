/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_DISPLAY_H_
#define _LIBDISPLAY_DISPLAY_H_

#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include "display/wndparams.h"
#include "display/wndresize.h"
#include "types/display.h"
#include "types/display/cursor.h"
#include "types/display/info.h"

extern errno_t display_open(const char *, display_t **);
extern void display_close(display_t *);
extern errno_t display_get_info(display_t *, display_info_t *);

extern errno_t display_window_create(display_t *, display_wnd_params_t *,
    display_wnd_cb_t *, void *, display_window_t **);
extern errno_t display_window_destroy(display_window_t *);
extern errno_t display_window_get_gc(display_window_t *, gfx_context_t **);
extern errno_t display_window_move_req(display_window_t *, gfx_coord2_t *,
    sysarg_t);
extern errno_t display_window_resize_req(display_window_t *,
    display_wnd_rsztype_t, gfx_coord2_t *, sysarg_t);
extern errno_t display_window_move(display_window_t *, gfx_coord2_t *);
extern errno_t display_window_get_pos(display_window_t *, gfx_coord2_t *);
extern errno_t display_window_get_max_rect(display_window_t *, gfx_rect_t *);
extern errno_t display_window_resize(display_window_t *,
    gfx_coord2_t *, gfx_rect_t *);
extern errno_t display_window_minimize(display_window_t *);
extern errno_t display_window_maximize(display_window_t *);
extern errno_t display_window_unmaximize(display_window_t *);
extern errno_t display_window_set_cursor(display_window_t *,
    display_stock_cursor_t);
extern errno_t display_window_set_caption(display_window_t *, const char *);

#endif

/** @}
 */
