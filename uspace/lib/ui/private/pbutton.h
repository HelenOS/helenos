/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	const char *caption;
	/** Button is selected as default */
	bool isdefault;
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
