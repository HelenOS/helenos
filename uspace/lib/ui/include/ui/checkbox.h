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

/** @addtogroup libui
 * @{
 */
/**
 * @file Check box
 */

#ifndef _UI_CHECKBOX_H
#define _UI_CHECKBOX_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/checkbox.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <stdbool.h>

extern errno_t ui_checkbox_create(ui_resource_t *, const char *,
    ui_checkbox_t **);
extern void ui_checkbox_destroy(ui_checkbox_t *);
extern ui_control_t *ui_checkbox_ctl(ui_checkbox_t *);
extern void ui_checkbox_set_cb(ui_checkbox_t *, ui_checkbox_cb_t *, void *);
extern void ui_checkbox_set_rect(ui_checkbox_t *, gfx_rect_t *);
extern bool ui_checkbox_get_checked(ui_checkbox_t *);
extern void ui_checkbox_set_checked(ui_checkbox_t *, bool);
extern errno_t ui_checkbox_paint(ui_checkbox_t *);
extern void ui_checkbox_press(ui_checkbox_t *);
extern void ui_checkbox_release(ui_checkbox_t *);
extern void ui_checkbox_enter(ui_checkbox_t *);
extern void ui_checkbox_leave(ui_checkbox_t *);
extern void ui_checkbox_switched(ui_checkbox_t *);
extern ui_evclaim_t ui_checkbox_pos_event(ui_checkbox_t *, pos_event_t *);

#endif

/** @}
 */
