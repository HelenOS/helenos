/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file UI control structure
 *
 */

#ifndef _UI_PRIVATE_CONTROL_H
#define _UI_PRIVATE_CONTROL_H

#include <gfx/coord.h>
#include <stdbool.h>

/** Actual structure of UI control.
 *
 * UI control is the abstract base class to all UI controls (e.g. push button,
 * label). This is private to libui.
 */
struct ui_control {
	/** Pointer to layout element structure this control is attached to
	 * or @c NULL if the control is not attached to a layout
	 */
	void *elemp;
	/** Ops */
	struct ui_control_ops *ops;
	/** Extended data */
	void *ext;
};

#endif

/** @}
 */
