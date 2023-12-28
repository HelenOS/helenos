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
 * @file Window decoration
 */

#ifndef _UI_TYPES_WDECOR_H
#define _UI_TYPES_WDECOR_H

#include <gfx/coord.h>
#include <types/common.h>
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
	/** Window has a system menu handle */
	ui_wds_sysmenu_hdl = 0x4,
	/** Window has a minimize button */
	ui_wds_minimize_btn = 0x8,
	/** Window has a maximize button */
	ui_wds_maximize_btn = 0x10,
	/** Window has a close button */
	ui_wds_close_btn = 0x20,
	/** Window is resizable */
	ui_wds_resizable = 0x40,
	/** Window is decorated (default decoration) */
	ui_wds_decorated = ui_wds_frame | ui_wds_titlebar | ui_wds_sysmenu_hdl |
	    ui_wds_minimize_btn | ui_wds_close_btn
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
	void (*sysmenu_open)(ui_wdecor_t *, void *, sysarg_t);
	void (*sysmenu_left)(ui_wdecor_t *, void *, sysarg_t);
	void (*sysmenu_right)(ui_wdecor_t *, void *, sysarg_t);
	void (*sysmenu_accel)(ui_wdecor_t *, void *, char32_t, sysarg_t);
	void (*minimize)(ui_wdecor_t *, void *);
	void (*maximize)(ui_wdecor_t *, void *);
	void (*unmaximize)(ui_wdecor_t *, void *);
	void (*close)(ui_wdecor_t *, void *);
	void (*move)(ui_wdecor_t *, void *, gfx_coord2_t *, sysarg_t);
	void (*resize)(ui_wdecor_t *, void *, ui_wdecor_rsztype_t,
	    gfx_coord2_t *, sysarg_t);
	void (*set_cursor)(ui_wdecor_t *, void *, ui_stock_cursor_t);
} ui_wdecor_cb_t;

#endif

/** @}
 */
