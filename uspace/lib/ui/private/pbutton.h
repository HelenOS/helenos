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
 * @file Push button structure
 *
 */

#ifndef _UI_PRIVATE_PBUTTON_H
#define _UI_PRIVATE_PBUTTON_H

#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/pbutton.h>

/** Actual structure of push button.
 *
 * This is private to libui.
 */
struct ui_pbutton {
	/** Base control object */
	struct ui_control *control;
	/** UI resource */
	struct ui_resource *res;
	/** Callbacks */
	struct ui_pbutton_cb *cb;
	/** Callback argument */
	void *arg;
	/** Custom decoration ops or @c NULL */
	struct ui_pbutton_decor_ops *decor_ops;
	/** Decoration argument */
	void *decor_arg;
	/** Push button rectangle */
	gfx_rect_t rect;
	/** Caption */
	char *caption;
	/** Button is selected as default */
	bool isdefault;
	/** Button light is on */
	bool light;
	/** Button is currently held down */
	bool held;
	/** Pointer is currently inside */
	bool inside;
	/** Push button flags */
	ui_pbutton_flags_t flags;
};

#endif

/** @}
 */
