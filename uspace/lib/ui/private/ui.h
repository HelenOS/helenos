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
 * @file User interface
 *
 */

#ifndef _UI_PRIVATE_UI_H
#define _UI_PRIVATE_UI_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <display.h>
#include <fibril_synch.h>
#include <io/console.h>
#include <stdbool.h>
#include <types/common.h>

/** Actual structure of user interface.
 *
 * This is private to libui.
 */
struct ui {
	/** Console */
	console_ctrl_t *console;
	/** Console GC */
	struct console_gc *cgc;
	/** Display */
	display_t *display;
	/** UI rectangle (only used in fullscreen mode) */
	gfx_rect_t rect;
	/** Output owned by UI, clean up when destroying UI */
	bool myoutput;
	/** @c true iff UI is suspended */
	bool suspended;
	/** @c true if terminating */
	bool quit;
	/** Windows (in stacking order, ui_window_t) */
	list_t windows;
	/** UI lock */
	fibril_mutex_t lock;
	/** Clickmatic */
	struct ui_clickmatic *clickmatic;
	/** Default input device ID used to determine new window's seat */
	sysarg_t idev_id;
};

#endif

/** @}
 */
