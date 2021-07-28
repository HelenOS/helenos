/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @file Radio button
 */

#ifndef _UI_RBUTTON_H
#define _UI_RBUTTON_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/rbutton.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <stdbool.h>

extern errno_t ui_rbutton_group_create(ui_resource_t *,
    ui_rbutton_group_t **);
extern void ui_rbutton_group_destroy(ui_rbutton_group_t *);
extern errno_t ui_rbutton_create(ui_rbutton_group_t *, const char *, void *,
    ui_rbutton_t **);
extern void ui_rbutton_destroy(ui_rbutton_t *);
extern ui_control_t *ui_rbutton_ctl(ui_rbutton_t *);
extern void ui_rbutton_group_set_cb(ui_rbutton_group_t *, ui_rbutton_group_cb_t *,
    void *);
extern void ui_rbutton_set_rect(ui_rbutton_t *, gfx_rect_t *);
extern errno_t ui_rbutton_paint(ui_rbutton_t *);
extern void ui_rbutton_press(ui_rbutton_t *);
extern void ui_rbutton_release(ui_rbutton_t *);
extern void ui_rbutton_enter(ui_rbutton_t *);
extern void ui_rbutton_leave(ui_rbutton_t *);
extern void ui_rbutton_select(ui_rbutton_t *);
extern void ui_rbutton_selected(ui_rbutton_t *);
extern ui_evclaim_t ui_rbutton_pos_event(ui_rbutton_t *, pos_event_t *);

#endif

/** @}
 */
