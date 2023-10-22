/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file User interface
 */

#ifndef _UI_UI_H
#define _UI_UI_H

#include <display.h>
#include <errno.h>
#include <gfx/coord.h>
#include <io/console.h>
#include <stdbool.h>
#include <types/ui/clickmatic.h>
#include <types/ui/ui.h>

extern errno_t ui_create(const char *, ui_t **);
extern errno_t ui_create_cons(console_ctrl_t *, ui_t **);
extern errno_t ui_create_disp(display_t *, ui_t **);
extern void ui_destroy(ui_t *);
extern void ui_quit(ui_t *);
extern void ui_run(ui_t *);
extern errno_t ui_paint(ui_t *);
extern bool ui_is_textmode(ui_t *);
extern bool ui_is_fullscreen(ui_t *);
extern errno_t ui_get_rect(ui_t *, gfx_rect_t *);
extern errno_t ui_suspend(ui_t *);
extern errno_t ui_resume(ui_t *);
extern bool ui_is_suspended(ui_t *);
extern void ui_lock(ui_t *);
extern void ui_unlock(ui_t *);
extern ui_clickmatic_t *ui_get_clickmatic(ui_t *);

#endif

/** @}
 */
