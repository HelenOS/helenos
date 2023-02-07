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
 * @file Tab structure
 *
 */

#ifndef _UI_PRIVATE_TAB_H
#define _UI_PRIVATE_TAB_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/tab.h>
#include <types/ui/resource.h>

/** Actual structure of tab.
 *
 * This is private to libui.
 */
struct ui_tab {
	/** Containing tab set */
	struct ui_tab_set *tabset;
	/** Link to @c tabset->tabs */
	link_t ltabs;
	/** Caption */
	char *caption;
	/** X offset of the handle */
	gfx_coord_t xoff;
	/** Tab content */
	struct ui_control *content;
};

/** Tab geometry.
 *
 * Computed rectangles of tab elements.
 */
typedef struct {
	/** Tab handle */
	gfx_rect_t handle;
	/** Tab handle area including pull-up area */
	gfx_rect_t handle_area;
	/** Tab body */
	gfx_rect_t body;
	/** Text position */
	gfx_coord2_t text_pos;
} ui_tab_geom_t;

extern gfx_coord_t ui_tab_handle_width(ui_tab_t *);
extern gfx_coord_t ui_tab_handle_height(ui_tab_t *);
extern void ui_tab_get_geom(ui_tab_t *, ui_tab_geom_t *);
extern errno_t ui_tab_paint_handle_frame(gfx_context_t *, gfx_rect_t *,
    gfx_coord_t, gfx_color_t *, gfx_color_t *, bool, gfx_rect_t *);
extern errno_t ui_tab_paint_body_frame(ui_tab_t *);
extern errno_t ui_tab_paint_frame(ui_tab_t *);
extern ui_resource_t *ui_tab_get_res(ui_tab_t *);

#endif

/** @}
 */
