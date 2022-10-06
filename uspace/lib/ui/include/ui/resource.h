/*
 * Copyright (c) 2020 Jiri Svoboda
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
 * @file UI resources
 */

#ifndef _UI_RESOURCE_H
#define _UI_RESOURCE_H

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/font.h>
#include <stdbool.h>
#include <types/ui/resource.h>

extern errno_t ui_resource_create(gfx_context_t *, bool, ui_resource_t **);
extern void ui_resource_destroy(ui_resource_t *);
extern void ui_resource_set_expose_cb(ui_resource_t *, ui_expose_cb_t,
    void *);
extern void ui_resource_expose(ui_resource_t *);
extern gfx_font_t *ui_resource_get_font(ui_resource_t *);
extern bool ui_resource_is_textmode(ui_resource_t *);
extern gfx_color_t *ui_resource_get_wnd_face_color(ui_resource_t *);
extern gfx_color_t *ui_resource_get_wnd_text_color(ui_resource_t *);

#endif

/** @}
 */
