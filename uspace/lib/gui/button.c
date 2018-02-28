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
#include <stdlib.h>
#include <drawctx.h>
#include <surface.h>
#include <font/embedded.h>
#include <errno.h>
#include "common.h"
#include "window.h"
#include "button.h"

static pixel_t color_highlight = PIXEL(255, 255, 255, 255);
static pixel_t color_shadow = PIXEL(255, 85, 85, 85);

static void paint_internal(widget_t *widget)
{
	button_t *btn = (button_t *) widget;

	surface_t *surface = window_claim(btn->widget.window);
	if (!surface)
		window_yield(btn->widget.window);

	source_t source;
	source_init(&source);

	drawctx_t drawctx;
	drawctx_init(&drawctx, surface);

	drawctx_set_source(&drawctx, &btn->background);
	drawctx_transfer(&drawctx, widget->hpos, widget->vpos,
	    widget->width, widget->height);

	if ((widget->width >= 8) && (widget->height >= 8)) {
		drawctx_set_source(&drawctx, &source);
		draw_bevel(&drawctx, &source, widget->hpos + 3, widget->vpos + 3,
		    widget->width - 6, widget->height - 6, color_highlight,
		    color_shadow);

		drawctx_set_source(&drawctx, &btn->foreground);
		drawctx_transfer(&drawctx, widget->hpos + 4, widget->vpos + 4,
		    widget->width - 8, widget->height - 8);
	}

	sysarg_t cpt_width;
	sysarg_t cpt_height;
	font_get_box(btn->font, btn->caption, &cpt_width, &cpt_height);

	if ((widget->width >= cpt_width) && (widget->height >= cpt_height)) {
		sysarg_t x = ((widget->width - cpt_width) / 2) + widget->hpos;
		sysarg_t y = ((widget->height - cpt_height) / 2) + widget->vpos;

		drawctx_set_source(&drawctx, &btn->text);
		drawctx_set_font(&drawctx, btn->font);

		if (btn->caption)
			drawctx_print(&drawctx, btn->caption, x, y);
	}

	window_yield(btn->widget.window);
}

void deinit_button(button_t *btn)
{
	widget_deinit(&btn->widget);
	free(btn->caption);
	font_release(btn->font);
}

static void button_destroy(widget_t *widget)
{
	button_t *btn = (button_t *) widget;

	deinit_button(btn);
	free(btn);
}

static void button_reconfigure(widget_t *widget)
{
	/* no-op */
}

static void button_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	widget_modify(widget, hpos, vpos, width, height);
	paint_internal(widget);
}

static void button_repaint(widget_t *widget)
{
	paint_internal(widget);
	window_damage(widget->window);
}

static void button_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	button_t *btn = (button_t *) widget;

	if (event.key == KC_ENTER && event.type == KEY_PRESS)
		sig_send(&btn->clicked, NULL);
}

static void button_handle_position_event(widget_t *widget, pos_event_t event)
{
	button_t *btn = (button_t *) widget;
	widget->window->focus = widget;

	// TODO make the click logic more robust (mouse grabbing, mouse moves)
	if (event.btn_num == 1) {
		if (event.type == POS_RELEASE)
			sig_send(&btn->clicked, NULL);
	}
}

bool init_button(button_t *btn, widget_t *parent, const void *data,
    const char *caption, uint16_t points, pixel_t background,
    pixel_t foreground, pixel_t text)
{
	widget_init(&btn->widget, parent, data);

	btn->widget.destroy = button_destroy;
	btn->widget.reconfigure = button_reconfigure;
	btn->widget.rearrange = button_rearrange;
	btn->widget.repaint = button_repaint;
	btn->widget.handle_keyboard_event = button_handle_keyboard_event;
	btn->widget.handle_position_event = button_handle_position_event;

	source_init(&btn->background);
	source_set_color(&btn->background, background);

	source_init(&btn->foreground);
	source_set_color(&btn->foreground, foreground);

	source_init(&btn->text);
	source_set_color(&btn->text, text);

	if (caption == NULL)
		btn->caption = NULL;
	else
		btn->caption = str_dup(caption);

	errno_t rc = embedded_font_create(&btn->font, points);
	if (rc != EOK) {
		free(btn->caption);
		btn->caption = NULL;
		return false;
	}

	sysarg_t cpt_width;
	sysarg_t cpt_height;
	font_get_box(btn->font, btn->caption, &cpt_width, &cpt_height);
	btn->widget.width_min = cpt_width + 10;
	btn->widget.height_min = cpt_height + 10;
	btn->widget.width_ideal = cpt_width + 30;
	btn->widget.height_ideal = cpt_height + 10;

	return true;
}

button_t *create_button(widget_t *parent, const void *data, const char *caption,
    uint16_t points, pixel_t background, pixel_t foreground, pixel_t text)
{
	button_t *btn = (button_t *) malloc(sizeof(button_t));
	if (!btn)
		return NULL;

	if (init_button(btn, parent, data, caption, points, background, foreground,
	    text))
		return btn;

	free(btn);
	return NULL;
}

/** @}
 */
