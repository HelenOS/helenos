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

/** @addtogroup libui
 * @{
 */
/**
 * @file Tab
 */

#ifndef _UI_TAB_H
#define _UI_TAB_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/common.h>
#include <types/ui/control.h>
#include <types/ui/tab.h>
#include <types/ui/tabset.h>
#include <types/ui/event.h>
#include <uchar.h>

extern errno_t ui_tab_create(ui_tab_set_t *, const char *, ui_tab_t **);
extern void ui_tab_destroy(ui_tab_t *);
extern ui_tab_t *ui_tab_first(ui_tab_set_t *);
extern ui_tab_t *ui_tab_next(ui_tab_t *);
extern ui_tab_t *ui_tab_last(ui_tab_set_t *);
extern ui_tab_t *ui_tab_prev(ui_tab_t *);
extern bool ui_tab_is_selected(ui_tab_t *);
extern void ui_tab_add(ui_tab_t *, ui_control_t *);
extern void ui_tab_remove(ui_tab_t *, ui_control_t *);
extern errno_t ui_tab_paint(ui_tab_t *);
extern ui_evclaim_t ui_tab_kbd_event(ui_tab_t *, kbd_event_t *);
extern ui_evclaim_t ui_tab_pos_event(ui_tab_t *, pos_event_t *);

#endif

/** @}
 */
