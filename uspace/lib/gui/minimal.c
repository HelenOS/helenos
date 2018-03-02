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

#include <stdlib.h>
#include <surface.h>

#include "window.h"
#include "minimal.h"

static void paint_internal(widget_t *w)
{
	minimal_t *min = (minimal_t *) w;

	surface_t *surface = window_claim(min->widget.window);
	if (!surface) {
		window_yield(min->widget.window);
	}

	for (sysarg_t y = w->vpos; y <  w->vpos + w->height; ++y) {
		for (sysarg_t x = w->hpos; x < w->hpos + w->width; ++x) {
			if (y % 2) {
				if (x % 2) {
					surface_put_pixel(surface, x, y, min->pix_a);
				} else {
					surface_put_pixel(surface, x, y, min->pix_b);
				}
			} else {
				if (x % 2) {
					surface_put_pixel(surface, x, y, min->pix_b);
				} else {
					surface_put_pixel(surface, x, y, min->pix_a);
				}
			}
		}
	}

	window_yield(min->widget.window);
}

void deinit_minimal(minimal_t *min)
{
	widget_deinit(&min->widget);
}

static void minimal_destroy(widget_t *widget)
{
	minimal_t *min = (minimal_t *) widget;

	deinit_minimal(min);

	free(min);
}

static void minimal_reconfigure(widget_t *widget)
{
	/* no-op */
}

static void minimal_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	widget_modify(widget, hpos, vpos, width, height);
	paint_internal(widget);
}

static void minimal_repaint(widget_t *widget)
{
	paint_internal(widget);
	window_damage(widget->window);
}

static void minimal_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	/* no-op */
}

static void minimal_handle_position_event(widget_t *widget, pos_event_t event)
{
	/* no-op */
}

bool init_minimal(minimal_t *min, widget_t *parent, const void *data, pixel_t a,
    pixel_t b)
{
	widget_init(&min->widget, parent, data);

	min->widget.destroy = minimal_destroy;
	min->widget.reconfigure = minimal_reconfigure;
	min->widget.rearrange = minimal_rearrange;
	min->widget.repaint = minimal_repaint;
	min->widget.handle_keyboard_event = minimal_handle_keyboard_event;
	min->widget.handle_position_event = minimal_handle_position_event;

	min->pix_a = a;
	min->pix_b = b;

	return true;
}

minimal_t *create_minimal(widget_t *parent, const void *data, pixel_t a,
    pixel_t b)
{
	minimal_t *min = (minimal_t *) malloc(sizeof(minimal_t));
	if (!min) {
		return NULL;
	}

	if (init_minimal(min, parent, data, a, b)) {
		return min;
	} else {
		free(min);
		return NULL;
	}
}

/** @}
 */
