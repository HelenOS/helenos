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

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

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
#include <ui/pbutton.h>

#include "window.h"

typedef struct sButton Button;
struct sButton
{
	char* name;
	char* text;
	
	size_t max_length;
	
	ui_fixed_t *fixed;
	ui_pbutton_t *button;
	ui_pbutton_flags_t flags;
	
	gfx_rect_t rect;
	
	void (*set_text)(Button* w, char* text);
	char* (*get_text)(Button* w);	
	
	ui_pbutton_cb_t pbutton_cb;
	void (*set_callback)(Button* w, void (*pb_clicked)(ui_pbutton_t *, void *), void* extra);
	
	void (*update_rect)(Button* w);
	void (*set_flags)(Button* w, ui_pbutton_flags_t flags);
	void (*update_flags)(Button* w);
	void (*paint)(Button* w);
	
	void (*set_horizontal_align)(Button* w, gfx_halign_t a);
	void (*set_vertical_align)(Button* w, gfx_halign_t a);
	void (*destroy)(Button* w);
};

int init_button(Button* w, const char * name, Window* window);
void button_set_callback(Button* w, void (*pb_clicked)(ui_pbutton_t *, void *), void* extra);
void button_set_text(Button* w, char* text);
char* button_get_text(Button* w);
void button_update_rect(Button* w);
void button_set_flags(Button* w, ui_pbutton_flags_t flags);
void button_update_flags(Button* w);
void button_paint(Button* w);
void button_set_horizontal_align(Button* w, gfx_halign_t a);
void button_set_vertical_align(Button* w, gfx_halign_t a);
void button_destroy(Button* w);

void button_set_text(Button* w, char* text)
{
	//free(w->text);
	size_t szt = str_size(text) + 1;
	if ( szt > w->max_length )
		szt = w->max_length;
	w->text = malloc(sizeof(char) * szt);
	str_cpy(w->text, szt, text);
	ui_pbutton_set_caption(w->button, w->text);
	ui_pbutton_paint(w->button);
}

char* button_get_text(Button* w)
{
	return w->text;
}

void button_update_rect(Button* w)
{
	ui_pbutton_set_rect(w->button, &w->rect);
}

void button_paint(Button* w)
{
	ui_pbutton_paint(w->button);
}

void button_set_horizontal_align(Button* w, gfx_halign_t a)
{
	//ui_pbutton_set_halign(w->button, a);
}

void button_set_vertical_align(Button* w, gfx_halign_t a)
{
	//ui_pbutton_set_valign(w->button, a);
}

void button_set_callback(Button* w, void (*pb_clicked2)(ui_pbutton_t *, void *), void* extra)
{
	ui_pbutton_cb_t pbutton_cb = {
		.clicked = pb_clicked2
	};
	w->pbutton_cb = pbutton_cb;
	ui_pbutton_set_cb(w->button, &w->pbutton_cb, extra);
}

void button_set_flags(Button* w, ui_pbutton_flags_t flags)
{
	w->flags = flags;
	ui_pbutton_set_flags(w->button, w->flags);
}

void button_update_flags(Button* w)
{
	ui_pbutton_set_flags(w->button, w->flags);
}

void button_destroy(Button* w)
{
	ui_pbutton_destroy(w->button);
}

int init_button(Button* w, const char * name, Window* window)
{
	w->max_length = 50;
	w->text = malloc(sizeof(char) * 7);
	str_cpy(w->text, 7, (char*) "Button1");
	
	errno_t rc = ui_pbutton_create(window->ui_res, w->text, &w->button);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	/*w->rect.p0.x = 15;
	w->rect.p0.y = 130;
	w->rect.p1.x = 190;
	w->rect.p1.y = w->rect.p0.y + 28;*/
	ui_pbutton_set_rect(w->button, &w->rect);

	rc = ui_fixed_add(window->fixed, ui_pbutton_ctl(w->button));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}
	
	w->set_text = button_set_text;
	w->get_text = button_get_text;
	w->update_rect = button_update_rect;
	w->set_flags = button_set_flags;
	w->update_flags = button_update_flags;
	w->paint = button_paint;
	
	w->set_horizontal_align = button_set_horizontal_align;
	w->set_vertical_align = button_set_vertical_align;
	
	w->set_callback = button_set_callback;
	
	w->destroy = button_destroy;
	
	return EOK;
}

#endif
