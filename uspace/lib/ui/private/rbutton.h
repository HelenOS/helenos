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
 * @file Radio button structure
 *
 */

#ifndef _UI_PRIVATE_RBUTTON_H
#define _UI_PRIVATE_RBUTTON_H

#include <gfx/coord.h>
#include <stdbool.h>

/** Actual structure of radio button group.
 *
 * This is private to libui.
 */
struct ui_rbutton_group {
	/** UI resource */
	struct ui_resource *res;
	/** Callbacks */
	struct ui_rbutton_group_cb *cb;
	/** Callback argument */
	void *arg;
	/** Selected button */
	struct ui_rbutton *selected;
};

/** Actual structure of radio button.
 *
 * This is private to libui.
 */
struct ui_rbutton {
	/** Base control object */
	struct ui_control *control;
	/** Containing radio button group */
	struct ui_rbutton_group *group;
	/** Callback argument */
	void *arg;
	/** Radio button rectangle */
	gfx_rect_t rect;
	/** Caption */
	const char *caption;
	/** Radio button is currently held down */
	bool held;
	/** Pointer is currently inside */
	bool inside;
};

extern errno_t ui_rbutton_paint_gfx(ui_rbutton_t *);
extern errno_t ui_rbutton_paint_text(ui_rbutton_t *);

#endif

/** @}
 */
