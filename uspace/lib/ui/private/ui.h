/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	/** @c true if terminating */
	bool quit;
	/** Windows (in stacking order, ui_window_t) */
	list_t windows;
	/** UI lock */
	fibril_mutex_t lock;
	/** Clickmatic */
	struct ui_clickmatic *clickmatic;
};

#endif

/** @}
 */
