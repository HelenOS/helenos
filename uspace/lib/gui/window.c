/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup gui
 * @{
 */
/**
 * @file
 */

#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <as.h>
#include <stdlib.h>
#include <str.h>

#include <fibril.h>
#include <task.h>
#include <adt/prodcons.h>
#include <adt/list.h>

#include <loc.h>

#include <io/pixel.h>
#include <draw/source.h>
#include <draw/font.h>
#include <draw/drawctx.h>
#include <draw/surface.h>
#include <display.h>

#include "common.h"
#include "connection.h"
#include "widget.h"
#include "window.h"

static sysarg_t border_thickness = 4;
static sysarg_t bevel_thickness = 1;
static sysarg_t header_height = 20;
static sysarg_t header_min_width = 40;
static sysarg_t close_thickness = 20;
static sysarg_t corner_size = 24;
static sysarg_t window_initial_size = 1;

static pixel_t color_highlight = PIXEL(255, 255, 255, 255);
static pixel_t color_shadow = PIXEL(255, 85, 85, 85);
static pixel_t color_surface = PIXEL(255, 186, 186, 186);

static pixel_t color_header_focus_highlight = PIXEL(255, 120, 145, 255);
static pixel_t color_header_focus_shadow = PIXEL(255, 40, 48, 89);
static pixel_t color_header_focus_surface = PIXEL(255, 88, 106, 196);

static pixel_t color_header_unfocus_highlight = PIXEL(255, 16, 78, 126);
static pixel_t color_header_unfocus_shadow = PIXEL(255, 5, 26, 42);
static pixel_t color_header_unfocus_surface = PIXEL(255, 12, 57, 92);

static pixel_t color_caption_focus = PIXEL(255, 255, 255, 255);
static pixel_t color_caption_unfocus = PIXEL(255, 207, 207, 207);

static void window_close_event(void *);
static void window_focus_event(void *);
static void window_kbd_event(void *, kbd_event_t *);
static void window_pos_event(void *, pos_event_t *);
static void window_resize_event(void *, gfx_rect_t *);
static void window_unfocus_event(void *);

static display_wnd_cb_t window_cb = {
	.close_event = window_close_event,
	.focus_event = window_focus_event,
	.kbd_event = window_kbd_event,
	.pos_event = window_pos_event,
	.resize_event = window_resize_event,
	.unfocus_event = window_unfocus_event
};

static void set_cursor(window_t *window, display_stock_cursor_t cursor)
{
	if (cursor != window->cursor) {
		(void) display_window_set_cursor(window->dwindow, cursor);
		window->cursor = cursor;
	}
}

static void paint_internal(widget_t *widget)
{
	surface_t *surface = window_claim(widget->window);
	if (!surface)
		window_yield(widget->window);

	source_t source;
	source_init(&source);

	drawctx_t drawctx;
	drawctx_init(&drawctx, surface);
	drawctx_set_source(&drawctx, &source);

	/* Window border outer bevel */

	draw_bevel(&drawctx, &source, widget->vpos, widget->hpos,
	    widget->width, widget->height, color_highlight, color_shadow);

	/* Window border surface */

	source_set_color(&source, color_surface);
	drawctx_transfer(&drawctx, widget->hpos + 1, widget->vpos + 1,
	    widget->width - 2, 2);
	drawctx_transfer(&drawctx, widget->hpos + 1, widget->vpos + 1,
	    2, widget->height - 2);
	drawctx_transfer(&drawctx, widget->hpos + 1,
	    widget->vpos + widget->height - 3, widget->width - 2, 2);
	drawctx_transfer(&drawctx, widget->hpos + widget->width - 3,
	    widget->vpos + 1, 2, widget->height - 4);

	/* Window border inner bevel */

	draw_bevel(&drawctx, &source, widget->hpos + 3, widget->vpos + 3,
	    widget->width - 6, widget->height - 6, color_shadow,
	    color_highlight);

	/* Header bevel */

	sysarg_t header_hpos = widget->hpos + border_thickness;
	sysarg_t header_vpos = widget->vpos + border_thickness;
	sysarg_t header_width = widget->width - 2 * border_thickness -
	    close_thickness;

	draw_bevel(&drawctx, &source, header_hpos, header_vpos,
	    header_width, header_height, widget->window->is_focused ?
	    color_header_focus_highlight : color_header_unfocus_highlight,
	    widget->window->is_focused ?
	    color_header_focus_shadow : color_header_unfocus_shadow);

	/* Header surface */

	source_set_color(&source, widget->window->is_focused ?
	    color_header_focus_surface : color_header_unfocus_surface);
	drawctx_transfer(&drawctx, header_hpos + 1, header_vpos + 1,
	    header_width - 2, header_height - 2);

	/* Close button bevel */

	sysarg_t close_hpos = widget->hpos + widget->width -
	    border_thickness - close_thickness;
	sysarg_t close_vpos = widget->vpos + border_thickness;

	draw_bevel(&drawctx, &source, close_hpos, close_vpos,
	    close_thickness, close_thickness, color_highlight, color_shadow);

	/* Close button surface */

	source_set_color(&source, color_surface);
	drawctx_transfer(&drawctx, close_hpos + 1, close_vpos + 1,
	    close_thickness - 2, close_thickness - 2);

	/* Close button icon */

	draw_icon_cross(surface, close_hpos + 3, close_vpos + 3,
	    color_highlight, color_shadow);

	/* Window caption */

	font_t *font;
	errno_t rc = embedded_font_create(&font, 16);
	if (rc != EOK) {
		window_yield(widget->window);
		return;
	}

	drawctx_set_font(&drawctx, font);
	source_set_color(&source, widget->window->is_focused ?
	    color_caption_focus : color_caption_unfocus);

	sysarg_t cpt_width;
	sysarg_t cpt_height;
	font_get_box(font, widget->window->caption, &cpt_width, &cpt_height);

	bool draw_title =
	    (widget->width >= 2 * border_thickness + 2 * bevel_thickness +
	    close_thickness + cpt_width);
	if (draw_title) {
		sysarg_t cpt_x = ((widget->width - cpt_width) / 2) + widget->hpos;
		sysarg_t cpt_y = ((header_height - cpt_height) / 2) +
		    widget->vpos + border_thickness;

		if (widget->window->caption)
			drawctx_print(&drawctx, widget->window->caption, cpt_x, cpt_y);
	}

	font_release(font);
	window_yield(widget->window);
}

static void root_destroy(widget_t *widget)
{
	widget_deinit(widget);
}

static void root_reconfigure(widget_t *widget)
{
	if (widget->window->is_decorated) {
		list_foreach(widget->children, link, widget_t, child) {
			child->rearrange(child,
			    widget->hpos + border_thickness,
			    widget->vpos + border_thickness + header_height,
			    widget->width - 2 * border_thickness,
			    widget->height - 2 * border_thickness - header_height);
		}
	} else {
		list_foreach(widget->children, link, widget_t, child) {
			child->rearrange(child, widget->hpos, widget->vpos,
			    widget->width, widget->height);
		}
	}
}

static void root_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	widget_modify(widget, hpos, vpos, width, height);
	if (widget->window->is_decorated) {
		paint_internal(widget);
		list_foreach(widget->children, link, widget_t, child) {
			child->rearrange(child,
			    hpos + border_thickness,
			    vpos + border_thickness + header_height,
			    width - 2 * border_thickness,
			    height - 2 * border_thickness - header_height);
		}
	} else {
		list_foreach(widget->children, link, widget_t, child) {
			child->rearrange(child, hpos, vpos, width, height);
		}
	}
}

static void root_repaint(widget_t *widget)
{
	if (widget->window->is_decorated) {
		paint_internal(widget);
	}
	list_foreach(widget->children, link, widget_t, child) {
		child->repaint(child);
	}
	if (widget->window->is_decorated) {
		window_damage(widget->window);
	}
}

static void root_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	if (!list_empty(&widget->children)) {
		widget_t *child = (widget_t *) list_first(&widget->children);
		child->handle_keyboard_event(child, event);
	}
}

static void root_handle_position_event(widget_t *widget, pos_event_t event)
{
	gfx_coord2_t pos;

	if (widget->window->is_decorated) {
		sysarg_t width = widget->width;
		sysarg_t height = widget->height;

		bool btn_left = (event.btn_num == 1) && (event.type == POS_PRESS);

		bool left = (event.hpos < border_thickness);
		bool right = (event.hpos >= width - border_thickness);
		bool top = (event.vpos < border_thickness);
		bool bottom = (event.vpos >= height - border_thickness);
		bool edge = left || right || top || bottom;

		bool cleft = (event.hpos < corner_size);
		bool cright = (event.hpos >= width - corner_size);
		bool ctop = (event.vpos < corner_size);
		bool cbottom = (event.vpos >= height - corner_size);

		bool header = (event.hpos >= border_thickness) &&
		    (event.hpos < width - border_thickness) &&
		    (event.vpos >= border_thickness) &&
		    (event.vpos < border_thickness + header_height);
		bool close = (header) &&
		    (event.hpos >= width - border_thickness - close_thickness);

		bool isresize = widget->window->is_resizable;
		display_wnd_rsztype_t rsztype = 0;

		if (edge && ctop && cleft) {
			rsztype = display_wr_top_left;
		} else if (edge && cbottom && cleft) {
			rsztype = display_wr_bottom_left;
		} else if (edge && cbottom && cright) {
			rsztype = display_wr_bottom_right;
		} else if (edge && ctop && cright) {
			rsztype = display_wr_top_right;
		} else if (top) {
			rsztype = display_wr_top;
		} else if (left) {
			rsztype = display_wr_left;
		} else if (bottom) {
			rsztype = display_wr_bottom;
		} else if (right) {
			rsztype = display_wr_right;
		} else {
			isresize = false;
		}

		if (isresize) {
			(void) set_cursor(widget->window,
			    display_cursor_from_wrsz(rsztype));
		} else {
			(void) set_cursor(widget->window, dcurs_arrow);
		}

		pos.x = event.hpos;
		pos.y = event.vpos;

		if (isresize && btn_left) {
			(void) display_window_resize_req(
			    widget->window->dwindow, rsztype, &pos);
		} else if (close && btn_left) {
			window_close(widget->window);
		} else if (header && btn_left) {
			(void) display_window_move_req(widget->window->dwindow,
			    &pos);
		} else {
			list_foreach(widget->children, link, widget_t, child) {
				child->handle_position_event(child, event);
			}
		}
	} else {
		list_foreach(widget->children, link, widget_t, child) {
			child->handle_position_event(child, event);
		}
	}
}

static void deliver_keyboard_event(window_t *win, kbd_event_t event)
{
	if (win->focus) {
		win->focus->handle_keyboard_event(win->focus, event);
	} else {
		win->root.handle_keyboard_event(&win->root, event);
	}
}

static void deliver_position_event(window_t *win, pos_event_t event)
{
	if (win->grab) {
		win->grab->handle_position_event(win->grab, event);
	} else {
		win->root.handle_position_event(&win->root, event);
	}
}

static void handle_signal_event(window_t *win, signal_event_t event)
{
	widget_t *widget = (widget_t *) event.object;
	slot_t slot = (slot_t) event.slot;
	void *data = (void *) event.argument;

	slot(widget, data);

	free(data);
}

static void handle_resize(window_t *win, sysarg_t offset_x, sysarg_t offset_y,
    sysarg_t width, sysarg_t height, window_placement_flags_t placement_flags)
{
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_t *new_bitmap = NULL;
	gfx_coord2_t offs;
	gfx_coord2_t dpos;
	display_info_t dinfo;
	gfx_rect_t drect;
	gfx_rect_t nrect;
	errno_t rc;

	if (width < 2 * border_thickness + header_min_width)
		return;

	if (height < 2 * border_thickness + header_height)
		return;

	fibril_mutex_lock(&win->guard);

	/* Deallocate old bitmap. */
	if (win->bitmap != NULL) {
		gfx_bitmap_destroy(win->bitmap);
		win->bitmap = NULL;
	}

	/* Deallocate old surface. */
	if (win->surface != NULL) {
		surface_destroy(win->surface);
		win->surface = NULL;
	}

	/* Place window, if appropriate */
	if (placement_flags != WINDOW_PLACEMENT_ANY) {
		dpos.x = 0;
		dpos.y = 0;

		rc = display_get_info(win->display, &dinfo);
		if (rc != EOK) {
			fibril_mutex_unlock(&win->guard);
			return;
		}

		drect = dinfo.rect;

		if (placement_flags & WINDOW_PLACEMENT_LEFT)
			dpos.x = drect.p0.x;
		else if (placement_flags & WINDOW_PLACEMENT_CENTER_X)
			dpos.x = (drect.p0.x + drect.p1.x - width) / 2;
		else
			dpos.x = drect.p1.x - width;

		if (placement_flags & WINDOW_PLACEMENT_TOP)
			dpos.y = drect.p0.y;
		else if (placement_flags & WINDOW_PLACEMENT_CENTER_Y)
			dpos.y = (drect.p0.y + drect.p1.y - height) / 2;
		else
			dpos.y = drect.p1.y - height;

		(void) display_window_move(win->dwindow, &dpos);
	}

	/* Resize the display window. */
	offs.x = offset_x;
	offs.y = offset_y;
	nrect.p0.x = 0;
	nrect.p0.y = 0;
	nrect.p1.x = width;
	nrect.p1.y = height;

	rc = display_window_resize(win->dwindow, &offs, &nrect);
	if (rc != EOK) {
		fibril_mutex_unlock(&win->guard);
		return;
	}

	gfx_bitmap_params_init(&params);
#ifndef CONFIG_WIN_DOUBLE_BUF
	params.flags = bmpf_direct_output;
#else
	params.flags = 0;
#endif
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = width;
	params.rect.p1.y = height;

	rc = gfx_bitmap_create(win->gc, &params, NULL, &new_bitmap);
	if (rc != EOK) {
		if (rc == ENOTSUP) {
			/* Direct output is not supported */
			params.flags &= ~bmpf_direct_output;
			rc = gfx_bitmap_create(win->gc, &params, NULL, &new_bitmap);
			if (rc != EOK) {
				fibril_mutex_unlock(&win->guard);
				return;
			}
		}
	}

	rc = gfx_bitmap_get_alloc(new_bitmap, &alloc);
	if (rc != EOK) {
		fibril_mutex_unlock(&win->guard);
		return;
	}

	/* Allocate new surface. */
	surface_t *new_surface = surface_create(width, height, alloc.pixels,
	    SURFACE_FLAG_SHARED);
	if (!new_surface) {
		gfx_bitmap_destroy(new_bitmap);
		fibril_mutex_unlock(&win->guard);
		return;
	}

	/* Switch in new surface and bitmap. */
	win->surface = new_surface;
	win->bitmap = new_bitmap;
	fibril_mutex_unlock(&win->guard);

	/*
	 * Let all widgets in the tree alter their position and size.
	 * Widgets might also paint themselves onto the new surface.
	 */
	win->root.rearrange(&win->root, 0, 0, width, height);

	fibril_mutex_lock(&win->guard);
	surface_reset_damaged_region(win->surface);
	fibril_mutex_unlock(&win->guard);

	(void) gfx_bitmap_render(win->bitmap, NULL, NULL);
}

static void handle_refresh(window_t *win)
{
	win->root.repaint(&win->root);
}

static void handle_damage(window_t *win)
{
	sysarg_t x, y, width, height;
	gfx_rect_t rect;
	fibril_mutex_lock(&win->guard);
	surface_get_damaged_region(win->surface, &x, &y, &width, &height);
	surface_reset_damaged_region(win->surface);
	fibril_mutex_unlock(&win->guard);

	if (width > 0 && height > 0) {
		rect.p0.x = x;
		rect.p0.y = y;
		rect.p1.x = x + width;
		rect.p1.y = y + height;

		if (win->bitmap != NULL)
			(void) gfx_bitmap_render(win->bitmap, &rect, NULL);
	}
}

static void destroy_children(widget_t *widget)
{
	/* Recursively destroy widget tree in bottom-top order. */
	while (!list_empty(&widget->children)) {
		widget_t *child =
		    list_get_instance(list_first(&widget->children), widget_t, link);
		destroy_children(child);
		child->destroy(child);
	}
}

static void handle_close(window_t *win)
{
	destroy_children(&win->root);
	win->root.destroy(&win->root);
	win->grab = NULL;
	win->focus = NULL;

	gfx_bitmap_destroy(win->bitmap);

	/*
	 * XXX Here we should properly destroy the IPC GC. We only have
	 * the generic GC so either it would need to be cast back or
	 * GC needs a virtual destructor.
	 */

	display_window_destroy(win->dwindow);
	display_close(win->display);

	while (!list_empty(&win->events.list)) {
		window_event_t *event = (window_event_t *) list_first(&win->events.list);
		list_remove(&event->link);
		free(event);
	}

	if (win->surface) {
		surface_destroy(win->surface);
	}

	free(win->caption);

	free(win);
}

/* Window event loop. Runs in own dedicated fibril. */
static errno_t event_loop(void *arg)
{
	bool is_main = false;
	bool terminate = false;
	window_t *win = (window_t *) arg;

	while (true) {
		window_event_t *event = (window_event_t *) prodcons_consume(&win->events);

		switch (event->type) {
		case ET_KEYBOARD_EVENT:
			deliver_keyboard_event(win, event->data.kbd);
			break;
		case ET_POSITION_EVENT:
			deliver_position_event(win, event->data.pos);
			break;
		case ET_SIGNAL_EVENT:
			handle_signal_event(win, event->data.signal);
			break;
		case ET_WINDOW_RESIZE:
			handle_resize(win, event->data.resize.offset_x,
			    event->data.resize.offset_y, event->data.resize.width,
			    event->data.resize.height, event->data.resize.placement_flags);
			break;
		case ET_WINDOW_FOCUS:
			if (!win->is_focused) {
				win->is_focused = true;
				handle_refresh(win);
			}
			break;
		case ET_WINDOW_UNFOCUS:
			if (win->is_focused) {
				win->is_focused = false;
				handle_refresh(win);
			}
			break;
		case ET_WINDOW_REFRESH:
			handle_refresh(win);
			break;
		case ET_WINDOW_DAMAGE:
			handle_damage(win);
			break;
		case ET_WINDOW_CLOSE:
			is_main = win->is_main;
			handle_close(win);
			terminate = true;
			break;
		default:
			break;
		}

		free(event);
		if (terminate) {
			break;
		}
	}

	if (is_main) {
		exit(0); /* Terminate whole task. */
	}
	return 0;
}

window_t *window_open(const char *winreg, const void *data,
    window_flags_t flags, const char *caption)
{
	display_wnd_params_t wparams;

	window_t *win = (window_t *) calloc(1, sizeof(window_t));
	if (!win)
		return NULL;

	win->is_main = flags & WINDOW_MAIN;
	win->is_decorated = flags & WINDOW_DECORATED;
	win->is_resizable = flags & WINDOW_RESIZEABLE;
	win->is_focused = true;
	prodcons_initialize(&win->events);
	fibril_mutex_initialize(&win->guard);

	widget_init(&win->root, NULL, data);
	win->root.window = win;
	win->root.destroy = root_destroy;
	win->root.reconfigure = root_reconfigure;
	win->root.rearrange = root_rearrange;
	win->root.repaint = root_repaint;
	win->root.handle_keyboard_event = root_handle_keyboard_event;
	win->root.handle_position_event = root_handle_position_event;
	win->grab = NULL;
	win->focus = NULL;
	win->cursor = dcurs_arrow;

	/* Allocate resources for new surface. */
	win->surface = surface_create(window_initial_size,
	    window_initial_size, NULL, SURFACE_FLAG_SHARED);
	if (win->surface == NULL) {
		free(win);
		return NULL;
	}

	errno_t rc = display_open(winreg, &win->display);
	if (rc != EOK) {
		surface_destroy(win->surface);
		free(win);
		return NULL;
	}

	/* Window dimensions are not know at this time */
	display_wnd_params_init(&wparams);
	wparams.rect.p0.x = 0;
	wparams.rect.p0.y = 0;
	wparams.rect.p1.x = window_initial_size;
	wparams.rect.p1.y = window_initial_size;
	wparams.min_size.x = 2 * border_thickness + header_min_width;
	wparams.min_size.y = 2 * border_thickness + header_height;

	rc = display_window_create(win->display, &wparams, &window_cb,
	    (void *) win, &win->dwindow);
	if (rc != EOK) {
		display_close(win->display);
		surface_destroy(win->surface);
		free(win);
		return NULL;
	}

	rc = display_window_get_gc(win->dwindow, &win->gc);
	if (rc != EOK) {
		display_window_destroy(win->dwindow);
		display_close(win->display);
		surface_destroy(win->surface);
		free(win);
		return NULL;
	}

	if (caption == NULL)
		win->caption = NULL;
	else
		win->caption = str_dup(caption);

	return win;
}

void window_resize(window_t *win, sysarg_t offset_x, sysarg_t offset_y,
    sysarg_t width, sysarg_t height, window_placement_flags_t placement_flags)
{
	window_event_t *event = (window_event_t *) malloc(sizeof(window_event_t));
	if (event) {
		link_initialize(&event->link);
		event->type = ET_WINDOW_RESIZE;
		event->data.resize.offset_x = offset_x;
		event->data.resize.offset_y = offset_y;
		event->data.resize.width = width;
		event->data.resize.height = height;
		event->data.resize.placement_flags = placement_flags;
		prodcons_produce(&win->events, &event->link);
	}
}

errno_t window_set_caption(window_t *win, const char *caption)
{
	char *cap;

	if (caption == NULL) {
		win->caption = NULL;
	} else {
		cap = str_dup(caption);
		if (cap == NULL)
			return ENOMEM;
		free(win->caption);
		win->caption = cap;
	}

	win->is_focused = false;
	handle_refresh(win);

	return EOK;
}

void window_refresh(window_t *win)
{
	window_event_t *event = (window_event_t *) malloc(sizeof(window_event_t));
	if (event) {
		link_initialize(&event->link);
		event->type = ET_WINDOW_REFRESH;
		prodcons_produce(&win->events, &event->link);
	}
}

void window_damage(window_t *win)
{
	window_event_t *event = (window_event_t *) malloc(sizeof(window_event_t));
	if (event) {
		link_initialize(&event->link);
		event->type = ET_WINDOW_DAMAGE;
		prodcons_produce(&win->events, &event->link);
	}
}

widget_t *window_root(window_t *win)
{
	return &win->root;
}

void window_exec(window_t *win)
{
	fid_t ev_fid = fibril_create(event_loop, win);
	if (!ev_fid) {
		return;
	}
	fibril_add_ready(ev_fid);
}

surface_t *window_claim(window_t *win)
{
	fibril_mutex_lock(&win->guard);
	return win->surface;
}

void window_yield(window_t *win)
{
	fibril_mutex_unlock(&win->guard);
}

void window_close(window_t *win)
{
	window_event_t *event;

	event = (window_event_t *) calloc(1, sizeof(window_event_t));
	if (event == NULL)
		return;

	link_initialize(&event->link);
	event->type = ET_WINDOW_CLOSE;
	prodcons_produce(&win->events, &event->link);
}

static void window_close_event(void *arg)
{
	window_t *win = (window_t *) arg;

	window_close(win);
}

static void window_focus_event(void *arg)
{
	window_t *win = (window_t *) arg;
	window_event_t *event;

	event = (window_event_t *) calloc(1, sizeof(window_event_t));
	if (event == NULL)
		return;

	link_initialize(&event->link);
	event->type = ET_WINDOW_FOCUS;
	prodcons_produce(&win->events, &event->link);
}

static void window_kbd_event(void *arg, kbd_event_t *kevent)
{
	window_t *win = (window_t *) arg;
	window_event_t *event;

	event = (window_event_t *) calloc(1, sizeof(window_event_t));
	if (event == NULL)
		return;

	link_initialize(&event->link);
	event->type = ET_KEYBOARD_EVENT;
	event->data.kbd = *kevent;
	prodcons_produce(&win->events, &event->link);
}

static void window_pos_event(void *arg, pos_event_t *pevent)
{
	window_t *win = (window_t *) arg;
	window_event_t *event;

	event = (window_event_t *) calloc(1, sizeof(window_event_t));
	if (event == NULL)
		return;

	link_initialize(&event->link);
	event->type = ET_POSITION_EVENT;
	event->data.pos = *pevent;
	prodcons_produce(&win->events, &event->link);
}

static void window_resize_event(void *arg, gfx_rect_t *nrect)
{
	window_t *win = (window_t *) arg;
	window_event_t *event;

	if (!win->is_resizable)
		return;

	event = (window_event_t *) calloc(1, sizeof(window_event_t));
	if (event == NULL)
		return;

	link_initialize(&event->link);
	event->type = ET_WINDOW_RESIZE;
	event->data.resize.offset_x = nrect->p0.x;
	event->data.resize.offset_y = nrect->p0.y;
	event->data.resize.width = nrect->p1.x - nrect->p0.x;
	event->data.resize.height = nrect->p1.y - nrect->p0.y;
	event->data.resize.placement_flags = WINDOW_PLACEMENT_ANY;
	prodcons_produce(&win->events, &event->link);
}

static void window_unfocus_event(void *arg)
{
	window_t *win = (window_t *) arg;
	window_event_t *event;

	event = (window_event_t *) calloc(1, sizeof(window_event_t));
	if (event == NULL)
		return;

	link_initialize(&event->link);
	event->type = ET_WINDOW_UNFOCUS;
	prodcons_produce(&win->events, &event->link);
}

/** @}
 */
