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
 * @file Popup window
 */

#ifndef _UI_POPUP_H
#define _UI_POPUP_H

#include <errno.h>
#include <gfx/context.h>
#include <types/ui/control.h>
#include <types/ui/ui.h>
#include <types/ui/popup.h>
#include <types/ui/resource.h>
#include <types/ui/window.h>

extern void ui_popup_params_init(ui_popup_params_t *);
extern errno_t ui_popup_create(ui_t *, ui_window_t *, ui_popup_params_t *,
    ui_popup_t **);
extern void ui_popup_set_cb(ui_popup_t *, ui_popup_cb_t *, void *);
extern void ui_popup_destroy(ui_popup_t *);
extern void ui_popup_add(ui_popup_t *, ui_control_t *);
extern void ui_popup_remove(ui_popup_t *, ui_control_t *);
extern ui_resource_t *ui_popup_get_res(ui_popup_t *);
extern gfx_context_t *ui_popup_get_gc(ui_popup_t *);
extern sysarg_t ui_popup_get_idev_id(ui_popup_t *);

#endif

/** @}
 */
