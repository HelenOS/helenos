/*
 * Copyright (c) 2024 Jiri Svoboda
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
 * @file Taskbar start menu
 */

#ifndef TBSMENU_H
#define TBSMENU_H

#include <errno.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <stddef.h>
#include <ui/fixed.h>
#include <ui/window.h>
#include <wndmgt.h>
#include "types/tbsmenu.h"

extern errno_t tbsmenu_create(ui_window_t *, ui_fixed_t *, const char *,
    tbsmenu_t **);
extern errno_t tbsmenu_load(tbsmenu_t *, const char *);
extern void tbsmenu_reload(tbsmenu_t *);
extern void tbsmenu_set_rect(tbsmenu_t *, gfx_rect_t *);
extern void tbsmenu_open(tbsmenu_t *);
extern void tbsmenu_close(tbsmenu_t *);
extern bool tbsmenu_is_open(tbsmenu_t *);
extern void tbsmenu_destroy(tbsmenu_t *);
extern errno_t tbsmenu_add(tbsmenu_t *, const char *, const char *, bool,
    tbsmenu_entry_t **);
extern errno_t tbsmenu_add_sep(tbsmenu_t *, tbsmenu_entry_t **);
extern void tbsmenu_remove(tbsmenu_t *, tbsmenu_entry_t *, bool);
extern tbsmenu_entry_t *tbsmenu_first(tbsmenu_t *);
extern tbsmenu_entry_t *tbsmenu_last(tbsmenu_t *);
extern tbsmenu_entry_t *tbsmenu_next(tbsmenu_entry_t *);
extern size_t tbsmenu_count(tbsmenu_t *);

#endif

/** @}
 */
