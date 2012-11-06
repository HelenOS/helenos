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

#include <assert.h>
#include <mem.h>
#include <malloc.h>
#include <surface.h>

#include "window.h"
#include "grid.h"

static void paint_internal(widget_t *w)
{
	grid_t *grid = (grid_t *) w;

	surface_t *surface = window_claim(grid->widget.window);
	if (!surface) {
		window_yield(grid->widget.window);
	}

	for (sysarg_t y = w->vpos; y <  w->vpos + w->height; ++y) {
		for (sysarg_t x = w->hpos; x < w->hpos + w->width; ++x) {
			surface_put_pixel(surface, x, y, grid->background);
		}
	}

	window_yield(grid->widget.window);
}

static widget_t **widget_at(grid_t *grid, size_t row, size_t col)
{
	if (row < grid->rows && col < grid->cols) {
		return grid->layout + (row * grid->cols + col);
	} else {
		return NULL;
	}
}

void deinit_grid(grid_t *grid)
{
	widget_deinit(&grid->widget);
	free(grid->layout);
}

static void grid_destroy(widget_t *widget)
{
	grid_t *grid = (grid_t *) widget;

	deinit_grid(grid);

	free(grid);
}

static void grid_reconfigure(widget_t *widget)
{
	/* no-op */
}

static void grid_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	grid_t *grid = (grid_t *) widget;

	widget_modify(widget, hpos, vpos, width, height);
	paint_internal(widget);

	sysarg_t cell_width = width / grid->cols;
	sysarg_t cell_height = height / grid->rows;

	list_foreach(widget->children, link) {
		widget_t *child = list_get_instance(link, widget_t, link);

		sysarg_t widget_hpos = 0;
		sysarg_t widget_vpos = 0;
		sysarg_t widget_width = 0;
		sysarg_t widget_height = 0;

		size_t r = 0;
		size_t c = 0;
		for (r = 0; r < grid->rows; ++r) {
			bool found = false;
			for (c = 0; c < grid->cols; ++c) {
				widget_t **cell = widget_at(grid, r, c);
				if (cell && *cell == child) {
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
		}

		widget_hpos = cell_width * c + hpos;
		widget_vpos = cell_height * r + vpos;

		for (size_t _c = c; _c < grid->cols; ++_c) {
			widget_t **cell = widget_at(grid, r, _c);
			if (cell && *cell == child) {
				widget_width += cell_width;
			} else {
				break;
			}
		}

		for (size_t _r = r; _r < grid->rows; ++_r) {
			widget_t **cell = widget_at(grid, _r, c);
			if (cell && *cell == child) {
				widget_height += cell_height;
			} else {
				break;
			}
		}

		if (widget_width > 0 && widget_height > 0) {
			child->rearrange(child,
			    widget_hpos, widget_vpos, widget_width, widget_height);
		}
	}
}

static void grid_repaint(widget_t *widget)
{
	paint_internal(widget);
	list_foreach(widget->children, link) {
		widget_t *child = list_get_instance(link, widget_t, link);
		child->repaint(child);
	}
	window_damage(widget->window);
}

static void grid_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	/* no-op */
}

static void grid_handle_position_event(widget_t *widget, pos_event_t event)
{
	grid_t *grid = (grid_t *) widget;

	if ((widget->height / grid->rows) == 0) {
		return;
	}
	if ((widget->width / grid->cols) == 0) {
		return;
	}

	sysarg_t row = (event.vpos - widget->vpos) / (widget->height / grid->rows);
	sysarg_t col = (event.hpos - widget->hpos) / (widget->width / grid->cols);

	widget_t **cell = widget_at(grid, row, col);
	if (cell && *cell) {
		(*cell)->handle_position_event(*cell, event);
	}
}

static void grid_add(grid_t *grid, widget_t *widget,
    size_t row, size_t col, size_t rows, size_t cols)
{
	assert(row + rows <= grid->rows);
	assert(col + cols <= grid->cols);

	widget->parent = (widget_t *) grid;
	list_append(&widget->link, &grid->widget.children);
	widget->window = grid->widget.window;

	for (size_t r = row; r < row + rows; ++r) {
		for (size_t c = col; c < col + cols; ++c) {
			widget_t **cell = widget_at(grid, r, c);
			if (cell) {
				*cell = widget;
			}
		}
	}
}

bool init_grid(grid_t *grid,
    widget_t *parent, size_t rows, size_t cols, pixel_t background)
{
	assert(rows > 0);
	assert(cols > 0);

	widget_t **layout = (widget_t **) malloc(rows * cols * sizeof(widget_t *));
	if (!layout) {
		return false;
	}
	memset(layout, 0, rows * cols * sizeof(widget_t *));

	widget_init(&grid->widget, parent);

	grid->widget.destroy = grid_destroy;
	grid->widget.reconfigure = grid_reconfigure;
	grid->widget.rearrange = grid_rearrange;
	grid->widget.repaint = grid_repaint;
	grid->widget.handle_keyboard_event = grid_handle_keyboard_event;
	grid->widget.handle_position_event = grid_handle_position_event;

	grid->add = grid_add;
	grid->background = background;
	grid->rows = rows;
	grid->cols = cols;
	grid->layout = layout;

	return true;
}

grid_t *create_grid(widget_t *parent, size_t rows, size_t cols, pixel_t background)
{
	grid_t *grid = (grid_t *) malloc(sizeof(grid_t));
	if (!grid) {
		return NULL;
	}

	if (init_grid(grid, parent, rows, cols, background)) {
		return grid;
	} else {
		free(grid);
		return NULL;
	}
}

/** @}
 */
