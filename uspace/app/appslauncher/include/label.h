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

#ifndef UI_LABEL_H
#define UI_LABEL_H

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
#include <ui/label.h>

#include "window.h"

typedef struct sLabel Label;
struct sLabel
{
	char* name;
	char* text;
	
	size_t max_length;
	
	ui_fixed_t *fixed;
	ui_label_t *label;
	
	gfx_rect_t rect;
	
	void (*set_text)(Label* w, char* text);
	char* (*get_text)(Label* w);
	
	void (*update_rect)(Label* w);
	void (*paint)(Label* w);
	
	void (*set_horizontal_align)(Label* w, gfx_halign_t a);
	void (*set_vertical_align)(Label* w, gfx_halign_t a);
	void (*destroy)(Label* w);
};

int init_label(Label* w, const char * name, Window* window);
void label_set_text(Label* w, char* text);
char* label_get_text(Label* w);
void label_update_rect(Label* w);
void label_paint(Label* w);
void label_set_horizontal_align(Label* w, gfx_halign_t a);
void label_set_vertical_align(Label* w, gfx_halign_t a);
void label_destroy(Label* w);

void label_set_text(Label* w, char* text)
{
	//free(&w->text);
	size_t szt = str_size(text) + 1;
	if ( szt > w->max_length )
		szt = w->max_length;
	w->text = malloc(sizeof(char) * szt);
	str_cpy(w->text, szt, text);
	ui_label_set_text(w->label, w->text);
	if(true)
		ui_label_paint(w->label);
}

char* label_get_text(Label* w)
{
	return w->text;
}

void label_update_rect(Label* w)
{
	ui_label_set_rect(w->label, &w->rect);
}

void label_paint(Label* w)
{
	ui_label_paint(w->label);
}

void label_set_horizontal_align(Label* w, gfx_halign_t a)
{
	ui_label_set_halign(w->label, a);
}

void label_set_vertical_align(Label* w, gfx_halign_t a)
{
	ui_label_set_valign(w->label, a);
}

void label_destroy(Label* w)
{
	ui_label_destroy(w->label);
}

int init_label(Label* w, const char * name, Window* window)
{
	w->max_length = 50;
	w->text = malloc(sizeof(char) * 6);
	str_cpy(w->text, 6, (char*) "Label1");
	
	errno_t rc = ui_label_create(window->ui_res, w->text, &w->label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	/*w->rect.p0.x = 0;
	w->rect.p0.y = 0;
	w->rect.p1.x = 0;
	w->rect.p1.y = 0;*/
	ui_label_set_rect(w->label, &w->rect);
	ui_label_set_halign(w->label, gfx_halign_left);

	rc = ui_fixed_add(window->fixed, ui_label_ctl(w->label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}
	
	w->set_text = label_set_text;
	w->get_text = label_get_text;
	w->update_rect = label_update_rect;
	w->paint = label_paint;
	
	w->set_horizontal_align = label_set_horizontal_align;
	w->set_vertical_align = label_set_vertical_align;
	
	w->destroy = label_destroy;
	
	return EOK;
}

#endif
