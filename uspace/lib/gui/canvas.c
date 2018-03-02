/*
 * Copyright (c) 2013 Martin Decky
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

#include <stdlib.h>
#include <transform.h>
#include <source.h>
#include <surface.h>
#include <drawctx.h>
#include "window.h"
#include "canvas.h"

static void paint_internal(widget_t *widget)
{
	canvas_t *canvas = (canvas_t *) widget;

	surface_t *surface = window_claim(canvas->widget.window);
	if (!surface) {
		window_yield(canvas->widget.window);
	}

	transform_t transform;
	transform_identity(&transform);
	transform_translate(&transform, widget->hpos, widget->vpos);

	source_t source;
	source_init(&source);
	source_set_transform(&source, transform);
	source_set_texture(&source, canvas->surface,
	    PIXELMAP_EXTEND_TRANSPARENT_BLACK);

	drawctx_t drawctx;
	drawctx_init(&drawctx, surface);

	drawctx_set_source(&drawctx, &source);
	drawctx_transfer(&drawctx, widget->hpos, widget->vpos, widget->width,
	    widget->height);

	window_yield(canvas->widget.window);
}

void deinit_canvas(canvas_t *canvas)
{
	widget_deinit(&canvas->widget);
}

static void canvas_destroy(widget_t *widget)
{
	canvas_t *canvas = (canvas_t *) widget;

	deinit_canvas(canvas);
	free(canvas);
}

static void canvas_reconfigure(widget_t *widget)
{
	/* No-op */
}

static void canvas_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	canvas_t *canvas = (canvas_t *) widget;

	widget_modify(widget, hpos, vpos, canvas->width, canvas->height);
	paint_internal(widget);
}

static void canvas_repaint(widget_t *widget)
{
	paint_internal(widget);
	window_damage(widget->window);
}

static void canvas_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	canvas_t *canvas = (canvas_t *) widget;

	sig_send(&canvas->keyboard_event, &event);
}

static void canvas_handle_position_event(widget_t *widget, pos_event_t event)
{
	canvas_t *canvas = (canvas_t *) widget;
	pos_event_t tevent;

	tevent = event;
	tevent.hpos -= widget->hpos;
	tevent.vpos -= widget->vpos;

	sig_send(&canvas->position_event, &tevent);
}

bool init_canvas(canvas_t *canvas, widget_t *parent, const void *data,
    sysarg_t width, sysarg_t height, surface_t *surface)
{
	widget_init(&canvas->widget, parent, data);

	canvas->widget.width = width;
	canvas->widget.height = height;

	canvas->widget.width_min = width;
	canvas->widget.height_min = height;
	canvas->widget.width_ideal = width;
	canvas->widget.height_ideal = height;
	canvas->widget.width_max = width;
	canvas->widget.height_max = height;

	canvas->widget.destroy = canvas_destroy;
	canvas->widget.reconfigure = canvas_reconfigure;
	canvas->widget.rearrange = canvas_rearrange;
	canvas->widget.repaint = canvas_repaint;
	canvas->widget.handle_keyboard_event = canvas_handle_keyboard_event;
	canvas->widget.handle_position_event = canvas_handle_position_event;

	canvas->width = width;
	canvas->height = height;
	canvas->surface = surface;

	return true;
}

bool update_canvas(canvas_t *canvas, surface_t *surface)
{
	if (surface != NULL)
		canvas->surface = surface;

	canvas_repaint(&canvas->widget);
	return true;
}

canvas_t *create_canvas(widget_t *parent, const void *data, sysarg_t width,
    sysarg_t height, surface_t *surface)
{
	canvas_t *canvas = (canvas_t *) malloc(sizeof(canvas_t));
	if (!canvas)
		return NULL;

	if (init_canvas(canvas, parent, data, width, height, surface))
		return canvas;

	free(canvas);
	return NULL;
}

/** @}
 */
