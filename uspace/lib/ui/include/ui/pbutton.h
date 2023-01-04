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
 * @file Push button
 */

#ifndef _UI_PBUTTON_H
#define _UI_PBUTTON_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/pbutton.h>
#include <types/ui/resource.h>
#include <stdbool.h>

extern errno_t ui_pbutton_create(ui_resource_t *, const char *,
    ui_pbutton_t **);
extern void ui_pbutton_destroy(ui_pbutton_t *);
extern ui_control_t *ui_pbutton_ctl(ui_pbutton_t *);
extern void ui_pbutton_set_cb(ui_pbutton_t *, ui_pbutton_cb_t *, void *);
extern void ui_pbutton_set_decor_ops(ui_pbutton_t *, ui_pbutton_decor_ops_t *,
    void *);
extern void ui_pbutton_set_flags(ui_pbutton_t *, ui_pbutton_flags_t);
extern void ui_pbutton_set_rect(ui_pbutton_t *, gfx_rect_t *);
extern void ui_pbutton_set_default(ui_pbutton_t *, bool);
extern bool ui_pbutton_get_light(ui_pbutton_t *);
extern void ui_pbutton_set_light(ui_pbutton_t *, bool);
extern errno_t ui_pbutton_set_caption(ui_pbutton_t *, const char *);
extern errno_t ui_pbutton_paint(ui_pbutton_t *);
extern void ui_pbutton_press(ui_pbutton_t *);
extern void ui_pbutton_release(ui_pbutton_t *);
extern void ui_pbutton_enter(ui_pbutton_t *);
extern void ui_pbutton_leave(ui_pbutton_t *);
extern void ui_pbutton_clicked(ui_pbutton_t *);
extern void ui_pbutton_down(ui_pbutton_t *);
extern void ui_pbutton_up(ui_pbutton_t *);
extern ui_evclaim_t ui_pbutton_pos_event(ui_pbutton_t *, pos_event_t *);

#endif

/** @}
 */
