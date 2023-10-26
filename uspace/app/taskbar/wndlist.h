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

/** @addtogroup taskbar
 * @{
 */
/**
 * @file Taskbar window list
 */

#ifndef WNDLIST_H
#define WNDLIST_H

#include <errno.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <stddef.h>
#include <ui/fixed.h>
#include <ui/window.h>
#include <wndmgt.h>
#include "types/wndlist.h"

extern errno_t wndlist_create(ui_window_t *, ui_fixed_t *, wndlist_t **);
extern void wndlist_set_rect(wndlist_t *, gfx_rect_t *);
extern errno_t wndlist_open_wm(wndlist_t *, const char *);
extern void wndlist_destroy(wndlist_t *);
extern errno_t wndlist_append(wndlist_t *, sysarg_t, const char *, bool,
    bool);
extern errno_t wndlist_remove(wndlist_t *, wndlist_entry_t *, bool);
extern bool wndlist_update_pitch(wndlist_t *);
extern errno_t wndlist_update(wndlist_t *, wndlist_entry_t *, const char *,
    bool);
extern void wndlist_set_entry_rect(wndlist_t *, wndlist_entry_t *);
extern wndlist_entry_t *wndlist_entry_by_id(wndlist_t *, sysarg_t);
extern wndlist_entry_t *wndlist_first(wndlist_t *);
extern wndlist_entry_t *wndlist_last(wndlist_t *);
extern wndlist_entry_t *wndlist_next(wndlist_entry_t *);
extern size_t wndlist_count(wndlist_t *);
extern errno_t wndlist_repaint(wndlist_t *);
extern errno_t wndlist_paint_entry(wndlist_entry_t *);
extern errno_t wndlist_unpaint_entry(wndlist_entry_t *);

#endif

/** @}
 */
