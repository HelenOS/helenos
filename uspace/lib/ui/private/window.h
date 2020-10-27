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
 * @file Window structure
 *
 */

#ifndef _UI_PRIVATE_WINDOW_H
#define _UI_PRIVATE_WINDOW_H

#include <display.h>
#include <gfx/context.h>
#include <io/pos_event.h>

/** Actual structure of window.
 *
 * This is private to libui.
 */
struct ui_window {
	/** Containing user interface */
	struct ui *ui;
	/** Callbacks */
	struct ui_window_cb *cb;
	/** Callback argument */
	void *arg;
	/** Display window */
	display_window_t *dwindow;
	/** Window GC */
	gfx_context_t *gc;
	/** UI resource. Ideally this would be in ui_t. */
	struct ui_resource *res;
	/** Window decoration */
	struct ui_wdecor *wdecor;
};

extern void ui_window_close(ui_window_t *);
extern void ui_window_pos(ui_window_t *, pos_event_t *);

#endif

/** @}
 */
