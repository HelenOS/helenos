/*
 * Copyright (c) 2012 Petr Koupy
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

#include <assert.h>
#include <mem.h>
#include <stdlib.h>
#include <surface.h>
#include "window.h"
#include "grid.h"

typedef struct {
	sysarg_t min;
	sysarg_t max;
	sysarg_t val;
} constraints_t;

static void paint_internal(widget_t *widget)
{
	grid_t *grid = (grid_t *) widget;

	surface_t *surface = window_claim(grid->widget.window);
	if (!surface) {
		window_yield(grid->widget.window);
		return;
	}

	// FIXME: Replace with (accelerated) rectangle fill
	for (sysarg_t y = widget->vpos; y < widget->vpos + widget->height; y++) {
		for (sysarg_t x = widget->hpos; x < widget->hpos + widget->width; x++)
			surface_put_pixel(surface, x, y, grid->background);
	}

	window_yield(grid->widget.window);
}

static grid_cell_t *grid_cell_at(grid_t *grid, size_t col, size_t row)
{
	if ((col < grid->cols) && (row < grid->rows))
		return grid->layout + (row * grid->cols + col);

	return NULL;
}

static grid_cell_t *grid_coords_at(grid_t *grid, sysarg_t hpos, sysarg_t vpos)
{
	for (size_t c = 0; c < grid->cols; c++) {
		for (size_t r = 0; r < grid->rows; r++) {
			grid_cell_t *cell = grid_cell_at(grid, c, r);
			if (cell) {
				widget_t *widget = cell->widget;

				if ((widget) && (hpos >= widget->hpos) &&
				    (vpos >= widget->vpos) &&
				    (hpos < widget->hpos + widget->width) &&
				    (vpos < widget->vpos + widget->height))
					return cell;
			}
		}
	}

	return NULL;
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
	/* No-op */
}

static void adjust_constraints(constraints_t *cons, size_t run,
    sysarg_t dim_min, sysarg_t dim_max)
{
	assert(run > 0);

	sysarg_t dim_min_part = dim_min / run;
	sysarg_t dim_min_rem = dim_min % run;

	sysarg_t dim_max_part = dim_max / run;
	sysarg_t dim_max_rem = dim_max % run;

	for (size_t i = 0; i < run; i++) {
		sysarg_t dim_min_cur = dim_min_part;
		sysarg_t dim_max_cur = dim_max_part;

		if (i == run - 1) {
			dim_min_cur += dim_min_rem;
			dim_max_cur += dim_max_rem;
		}

		/*
		 * We want the strongest constraint
		 * for the minimum.
		 */
		if (cons[i].min < dim_min_cur)
			cons[i].min = dim_min_cur;

		/*
		 * The comparison is correct, we want
		 * the weakest constraint for the
		 * maximum.
		 */
		if (cons[i].max < dim_max_cur)
			cons[i].max = dim_max_cur;
	}
}

static void solve_constraints(constraints_t *cons, size_t run, sysarg_t sum)
{
	/* Initial solution */
	sysarg_t cur_sum = 0;

	for (size_t i = 0; i < run; i++) {
		cons[i].val = cons[i].min;
		cur_sum += cons[i].val;
	}

	/* Iterative improvement */
	while (cur_sum < sum) {
		sysarg_t delta = (sum - cur_sum) / run;
		if (delta == 0)
			break;

		cur_sum = 0;

		for (size_t i = 0; i < run; i++) {
			if (cons[i].val + delta < cons[i].max)
				cons[i].val += delta;

			cur_sum += cons[i].val;
		}
	}
}

static void grid_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	grid_t *grid = (grid_t *) widget;

	widget_modify(widget, hpos, vpos, width, height);
	paint_internal(widget);

	/* Compute column widths */
	constraints_t *widths =
	    (constraints_t *) calloc(grid->cols, sizeof(constraints_t));
	if (widths) {
		/* Constrain widths */
		for (size_t c = 0; c < grid->cols; c++) {
			widths[c].min = 0;

			for (size_t r = 0; r < grid->rows; r++) {
				grid_cell_t *cell = grid_cell_at(grid, c, r);
				if (!cell)
					continue;

				widget_t *widget = cell->widget;
				if (widget)
					adjust_constraints(&widths[c], cell->cols,
					    widget->width_min, widget->width_max);
			}
		}

		solve_constraints(widths, grid->cols, width);
	}

	/* Compute row heights */
	constraints_t *heights =
	    (constraints_t *) calloc(grid->rows, sizeof(constraints_t));
	if (heights) {
		/* Constrain heights */
		for (size_t r = 0; r < grid->rows; r++) {
			heights[r].min = 0;

			for (size_t c = 0; c < grid->cols; c++) {
				grid_cell_t *cell = grid_cell_at(grid, c, r);
				if (!cell)
					continue;

				widget_t *widget = cell->widget;
				if (widget) {
					adjust_constraints(&heights[r], cell->rows,
					    widget->height_min, widget->height_max);
				}
			}
		}

		solve_constraints(heights, grid->rows, height);
	}

	/* Rearrange widgets */
	if ((widths) && (heights)) {
		sysarg_t cur_vpos = vpos;

		for (size_t r = 0; r < grid->rows; r++) {
			sysarg_t cur_hpos = hpos;

			for (size_t c = 0; c < grid->cols; c++) {
				grid_cell_t *cell = grid_cell_at(grid, c, r);
				if (!cell)
					continue;

				widget_t *widget = cell->widget;
				if (widget) {
					sysarg_t cur_width = 0;
					sysarg_t cur_height = 0;

					for (size_t cd = 0; cd < cell->cols; cd++)
						cur_width += widths[c + cd].val;

					for (size_t rd = 0; rd < cell->rows; rd++)
						cur_height += heights[r + rd].val;

					if ((cur_width > 0) && (cur_height > 0)) {
						sysarg_t wwidth = cur_width;
						sysarg_t wheight = cur_height;

						/*
						 * Make sure the widget is respects its
						 * maximal constrains.
						 */

						if ((widget->width_max > 0) &&
						    (wwidth > widget->width_max))
							wwidth = widget->width_max;

						if ((widget->height_max > 0) &&
						    (wheight > widget->height_max))
							wheight = widget->height_max;

						widget->rearrange(widget, cur_hpos, cur_vpos,
						    wwidth, wheight);
					}


				}

				cur_hpos += widths[c].val;
			}

			cur_vpos += heights[r].val;
		}
	}

	if (widths)
		free(widths);

	if (heights)
		free(heights);
}

static void grid_repaint(widget_t *widget)
{
	paint_internal(widget);

	list_foreach(widget->children, link, widget_t, child) {
		child->repaint(child);
	}

	window_damage(widget->window);
}

static void grid_handle_keyboard_event(widget_t *widget, kbd_event_t event)
{
	/* No-op */
}

static void grid_handle_position_event(widget_t *widget, pos_event_t event)
{
	grid_t *grid = (grid_t *) widget;

	grid_cell_t *cell = grid_coords_at(grid, event.hpos, event.vpos);
	if ((cell) && (cell->widget))
		cell->widget->handle_position_event(cell->widget, event);
}

static bool grid_add(struct grid *grid, widget_t *widget, size_t col,
    size_t row, size_t cols, size_t rows)
{
	if ((cols == 0) || (rows == 0) || (col + cols > grid->cols) ||
	    (row + rows > grid->rows))
		return false;

	grid_cell_t *cell = grid_cell_at(grid, col, row);
	if (!cell)
		return false;

	/*
	 * Check whether the cell is not occupied by an
	 * extension of a different cell.
	 */
	if ((!cell->widget) && (cell->cols > 0) && (cell->rows > 0))
		return false;

	widget->parent = (widget_t *) grid;

	list_append(&widget->link, &grid->widget.children);
	widget->window = grid->widget.window;

	/* Mark cells in layout */
	for (size_t r = row; r < row + rows; r++) {
		for (size_t c = col; c < col + cols; c++) {
			if ((r == row) && (c == col)) {
				cell->widget = widget;
				cell->cols = cols;
				cell->rows = rows;
			} else {
				grid_cell_t *extension = grid_cell_at(grid, c, r);
				if (extension) {
					extension->widget = NULL;
					extension->cols = 1;
					extension->rows = 1;
				}
			}
		}
	}

	return true;
}

bool init_grid(grid_t *grid, widget_t *parent, const void *data, size_t cols,
    size_t rows, pixel_t background)
{
	if ((cols == 0) || (rows == 0))
		return false;

	grid->layout =
	    (grid_cell_t *) calloc(cols * rows, sizeof(grid_cell_t));
	if (!grid->layout)
		return false;

	memset(grid->layout, 0, cols * rows * sizeof(grid_cell_t));

	widget_init(&grid->widget, parent, data);

	grid->widget.destroy = grid_destroy;
	grid->widget.reconfigure = grid_reconfigure;
	grid->widget.rearrange = grid_rearrange;
	grid->widget.repaint = grid_repaint;
	grid->widget.handle_keyboard_event = grid_handle_keyboard_event;
	grid->widget.handle_position_event = grid_handle_position_event;

	grid->add = grid_add;
	grid->background = background;
	grid->cols = cols;
	grid->rows = rows;

	return true;
}

grid_t *create_grid(widget_t *parent, const void *data, size_t cols,
    size_t rows, pixel_t background)
{
	grid_t *grid = (grid_t *) malloc(sizeof(grid_t));
	if (!grid)
		return NULL;

	if (init_grid(grid, parent, data, cols, rows, background))
		return grid;

	free(grid);
	return NULL;
}

/** @}
 */
