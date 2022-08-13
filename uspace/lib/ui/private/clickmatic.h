/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Clickmatic structure
 *
 */

#ifndef _UI_PRIVATE_CLICKMATIC_H
#define _UI_PRIVATE_CLICKMATIC_H

#include <fibril_synch.h>
#include <gfx/coord.h>

/** Actual structure of clickmatic.
 *
 * This is private to libui.
 */
struct ui_clickmatic {
	/** Containing user interface */
	struct ui *ui;
	/** Callbacks */
	struct ui_clickmatic_cb *cb;
	/** Callback argument */
	void *arg;
	/** Click position */
	gfx_coord2_t pos;
	/** Timer */
	fibril_timer_t *timer;
};

extern void ui_clickmatic_clicked(ui_clickmatic_t *);

#endif

/** @}
 */
