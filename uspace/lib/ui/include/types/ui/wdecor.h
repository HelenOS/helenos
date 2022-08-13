/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Window decoration
 */

#ifndef _UI_TYPES_WDECOR_H
#define _UI_TYPES_WDECOR_H

#include <gfx/coord.h>
#include <types/ui/cursor.h>

struct ui_wdecor;
typedef struct ui_wdecor ui_wdecor_t;

/** Window decoration style */
typedef enum {
	/** No style bits */
	ui_wds_none = 0x0,
	/** Window has a frame */
	ui_wds_frame = 0x1,
	/** Window has a title bar */
	ui_wds_titlebar = 0x2,
	/** Window has a maximize button */
	ui_wds_maximize_btn = 0x4,
	/** Window has a close button */
	ui_wds_close_btn = 0x8,
	/** Window is resizable */
	ui_wds_resizable = 0x10,
	/** Window is decorated (default decoration) */
	ui_wds_decorated = ui_wds_frame | ui_wds_titlebar | ui_wds_close_btn
} ui_wdecor_style_t;

/** Window resize type */
typedef enum {
	ui_wr_none = 0,

	ui_wr_top = 0x1,
	ui_wr_left = 0x2,
	ui_wr_bottom = 0x4,
	ui_wr_right = 0x8,

	ui_wr_top_left = ui_wr_top | ui_wr_left,
	ui_wr_bottom_left = ui_wr_bottom | ui_wr_left,
	ui_wr_bottom_right = ui_wr_bottom | ui_wr_right,
	ui_wr_top_right = ui_wr_top | ui_wr_right
} ui_wdecor_rsztype_t;

/** Window decoration callbacks */
typedef struct ui_wdecor_cb {
	void (*maximize)(ui_wdecor_t *, void *);
	void (*unmaximize)(ui_wdecor_t *, void *);
	void (*close)(ui_wdecor_t *, void *);
	void (*move)(ui_wdecor_t *, void *, gfx_coord2_t *);
	void (*resize)(ui_wdecor_t *, void *, ui_wdecor_rsztype_t,
	    gfx_coord2_t *);
	void (*set_cursor)(ui_wdecor_t *, void *, ui_stock_cursor_t);
} ui_wdecor_cb_t;

#endif

/** @}
 */
