/*
 * Copyright (c) 2023 SimonJRiddix
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

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "vector.h"

#include <errno.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>
#include <ui/fixed.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>

static const char *display_spec = UI_DISPLAY_DEFAULT;

typedef struct sWindow Window;
struct sWindow
{
	vector controls;
	char * name;
	
	ui_t *ui;
	ui_resource_t *ui_res;
	ui_wnd_params_t params;
	ui_window_t *window;
	ui_fixed_t *fixed;
	
	int (*draw)(Window* w);
	void (*destroy)(Window* w);
};

int draw_window(Window* w);
void destroy_window(Window* w);
int init_window(Window* w, const char * name, const char * caption);

int draw_window(Window* w)
{
	ui_window_add(w->window, ui_fixed_ctl(w->fixed));

	errno_t rc = ui_window_paint(w->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}
	
	ui_run(w->ui);
	
	return EOK;
}

void destroy_window(Window* w)
{
	ui_window_destroy(w->window);
	ui_destroy(w->ui);
}

int init_window(Window* w, const char * name, const char * caption)
{
	//vector_init(&w->controls);
	//w->name = name;
	
	errno_t rc = ui_create(display_spec, &w->ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}
	
	ui_wnd_params_init(&w->params);
	w->params.caption = caption;
	w->params.placement = ui_wnd_place_bottom_left;
	w->params.rect.p0.x = 0;
	w->params.rect.p0.y = 0;
	w->params.rect.p1.x = 380;
	w->params.rect.p1.y = 450;
	w->params.style = ui_wds_frame;
	
	rc = ui_window_create(w->ui, &w->params, &w->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	w->ui_res = ui_window_get_res(w->window);
	
	rc = ui_fixed_create(&w->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}
	
	w->draw = draw_window;
	
	w->destroy = destroy_window;
	
	return EOK;
}

#endif
