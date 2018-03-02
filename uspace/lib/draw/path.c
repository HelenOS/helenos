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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <stdlib.h>

#include "path.h"

struct path {
	list_t list;
	double cur_x;
	double cur_y;
};

void path_init(path_t *path)
{
	list_initialize(&path->list);
	path->cur_x = 0;
	path->cur_y = 0;
}

void path_clear(path_t *path)
{
	while (!list_empty(&path->list)) {
		path_step_t *step = (path_step_t *) list_last(&path->list);
		list_remove(&step->link);
		free(step);
	}
	path->cur_x = 0;
	path->cur_y = 0;
}

void path_get_cursor(path_t *path, double *x, double *y)
{
	assert(x);
	assert(y);

	*x = path->cur_x;
	*y = path->cur_y;
}

void path_move_to(path_t *path, double dx, double dy)
{
	path_step_t *step = (path_step_t *) malloc(sizeof(path_step_t));
	if (step) {
		path->cur_x += dx;
		path->cur_y += dy;
		link_initialize(&step->link);
		step->type = PATH_STEP_MOVETO;
		step->to_x = path->cur_x;
		step->to_y = path->cur_y;
		list_append(&step->link, &path->list);

	}
}

void path_line_to(path_t *path, double dx, double dy)
{
	path_step_t *step = (path_step_t *) malloc(sizeof(path_step_t));
	if (step) {
		path->cur_x += dx;
		path->cur_y += dy;
		link_initialize(&step->link);
		step->type = PATH_STEP_LINETO;
		step->to_x = path->cur_x;
		step->to_y = path->cur_y;
		list_append(&step->link, &path->list);
	}
}

void path_rectangle(path_t *path, double x, double y, double width, double height)
{
	path_move_to(path, x, y);
	path_line_to(path, width, 0);
	path_line_to(path, 0, height);
	path_line_to(path, -width, 0);
	path_line_to(path, 0, -height);
}

/** @}
 */
