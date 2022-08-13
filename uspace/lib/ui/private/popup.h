/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Popup structure
 *
 */

#ifndef _UI_PRIVATE_POPUP_H
#define _UI_PRIVATE_POPUP_H

#include <gfx/coord.h>
#include <types/ui/window.h>

/** Actual structure of popup window.
 *
 * This is private to libui.
 */
struct ui_popup {
	/** Containing user interface */
	struct ui *ui;
	/** Callbacks */
	struct ui_popup_cb *cb;
	/** Callback argument */
	void *arg;
	/** Parent window */
	struct ui_window *parent;
	/** Window */
	struct ui_window *window;
	/** Placement rectangle */
	gfx_rect_t place;
};

#endif

/** @}
 */
