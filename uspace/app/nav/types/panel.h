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

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator panel types
 */

#ifndef TYPES_PANEL_H
#define TYPES_PANEL_H

#include <gfx/color.h>
#include <gfx/coord.h>
#include <ui/filelist.h>
#include <ui/window.h>
#include <stdint.h>

/** Navigator panel
 *
 * This is a custom UI control.
 */
typedef struct panel {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** Callbacks */
	struct panel_cb *cb;

	/** Callback argument */
	void *cb_arg;

	/** Panel rectangle */
	gfx_rect_t rect;

	/** Panel color */
	gfx_color_t *color;

	/** Active border color */
	gfx_color_t *act_border_color;

	/** @c true iff the panel is active */
	bool active;

	/** File list */
	ui_file_list_t *flist;
} panel_t;

/** Panel callbacks */
typedef struct panel_cb {
	/** Request panel activation */
	void (*activate_req)(void *, panel_t *);
} panel_cb_t;

#endif

/** @}
 */
