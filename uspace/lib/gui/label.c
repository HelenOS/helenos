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

#include <str.h>
#include <drawctx.h>
#include <stdlib.h>
#include <surface.h>
#include <font/embedded.h>
#include <errno.h>
#include "window.h"
#include "label.h"

static void paint_internal(widget_t *widget)
{
	label_t *lbl = (label_t *) widget;

	surface_t *surface = window_claim(lbl->widget.window);
	if (!surface)
		window_yield(lbl->widget.window);

	drawctx_t drawctx;
	drawctx_init(&drawctx, surface);

	drawctx_set_source(&drawctx, &lbl->background);
	drawctx_transfer(&drawctx, widget->hpos, widget->vpos, widget->width,
	    widget->height);

	sysarg_t cpt_width;
	sysarg_t cpt_height;
	font_get_box(lbl->font, lbl->caption, &cpt_width, &cpt_height);

	if ((widget->width >= cpt_width) && (widget->height >= cpt_height)) {
		sysarg_t x = ((widget->width - cpt_width) / 2) + widget->hpos;
		sysarg_t y = ((widget->height - cpt_height) / 2) + widget->vpos;

		drawctx_set_source(&drawctx, &lbl->text);
		drawctx_set_font(&drawctx, lbl->font);

		if (lbl->caption)
			drawctx_print(&drawctx, lbl->caption, x, y);
	}

	window_yield(lbl->widget.window);
}

static void on_rewrite(widget_t *widget, void *data)
{
	if (data != NULL) {
		label_t *lbl = (label_t *) widget;

		const char *new_caption = (const char *) data;
		lbl->caption = str_dup(new_caption);

		sysarg_t cpt_width;
		sysarg_t cpt_height;
		font_get_box(lbl->font, lbl->caption, &cpt_width, &cpt_height);

		lbl->widget.width_min = cpt_width + 4;
		lbl->widget.height_min = cpt_height + 4;
		lbl->widget.width_ideal = lbl->widget.width_min;
		lbl->widget.height_ideal = lbl->widget.height_min;

		window_refresh(lbl->widget.window);
	}
}

void deinit_label(label_t *lbl)
{
	widget_deinit(&lbl->widget);
	free(lbl->caption);
	font_release(lbl->font);
}

static void label_destroy(widget_t *widget)
{
	label_t *lbl = (label_t *) widget;

	deinit_label(lbl);
	free(lbl);
}

static void label_reconfigure(widget_t *widget)
{
	/* no-op */
}

static void label_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	widget_modify(widget, hpos, vpos, width, height);
	paint_internal(widget);
}

static void label_repaint(widget_t *widget)
{
	paint_internal(widget);
	window_damage(widget->window);
}

static void label_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	/* no-op */
}

static void label_handle_position_event(widget_t *widget, pos_event_t event)
{
	/* no-op */
}

bool init_label(label_t *lbl, widget_t *parent, const void *data,
    const char *caption, uint16_t points, pixel_t background, pixel_t text)
{
	widget_init(&lbl->widget, parent, data);

	lbl->widget.destroy = label_destroy;
	lbl->widget.reconfigure = label_reconfigure;
	lbl->widget.rearrange = label_rearrange;
	lbl->widget.repaint = label_repaint;
	lbl->widget.handle_keyboard_event = label_handle_keyboard_event;
	lbl->widget.handle_position_event = label_handle_position_event;

	source_init(&lbl->background);
	source_set_color(&lbl->background, background);

	source_init(&lbl->text);
	source_set_color(&lbl->text, text);

	if (caption == NULL)
		lbl->caption = NULL;
	else
		lbl->caption = str_dup(caption);

	errno_t rc = embedded_font_create(&lbl->font, points);
	if (rc != EOK) {
		free(lbl->caption);
		lbl->caption = NULL;
		return false;
	}

	sysarg_t cpt_width;
	sysarg_t cpt_height;
	font_get_box(lbl->font, lbl->caption, &cpt_width, &cpt_height);

	lbl->widget.width_min = cpt_width + 4;
	lbl->widget.height_min = cpt_height + 4;
	lbl->widget.width_ideal = lbl->widget.width_min;
	lbl->widget.height_ideal = lbl->widget.height_min;

	lbl->rewrite = on_rewrite;

	return true;
}

label_t *create_label(widget_t *parent, const void *data, const char *caption,
    uint16_t points, pixel_t background, pixel_t text)
{
	label_t *lbl = (label_t *) malloc(sizeof(label_t));
	if (!lbl)
		return NULL;

	if (init_label(lbl, parent, data, caption, points, background, text))
		return lbl;

	free(lbl);
	return NULL;
}

/** @}
 */
