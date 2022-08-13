/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Scrollbar
 */

#ifndef _UI_TYPES_SCROLLBAR_H
#define _UI_TYPES_SCROLLBAR_H

#include <gfx/coord.h>

struct ui_scrollbar;
typedef struct ui_scrollbar ui_scrollbar_t;

/** Scrollbar direction */
typedef enum {
	/** Horizontal */
	ui_sbd_horiz,
	/** Vertical */
	ui_sbd_vert
} ui_scrollbar_dir_t;

/** Scrollbar callbacks */
typedef struct ui_scrollbar_cb {
	void (*up)(ui_scrollbar_t *, void *);
	void (*down)(ui_scrollbar_t *, void *);
	void (*page_up)(ui_scrollbar_t *, void *);
	void (*page_down)(ui_scrollbar_t *, void *);
	void (*moved)(ui_scrollbar_t *, void *, gfx_coord_t);
} ui_scrollbar_cb_t;

#endif

/** @}
 */
