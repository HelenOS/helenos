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

#include "widget.h"

/** Link widget with parent and initialize default position and size. */
void widget_init(widget_t *widget, widget_t *parent, const void *data)
{
	link_initialize(&widget->link);
	list_initialize(&widget->children);

	if (parent) {
		widget->parent = parent;
		list_append(&widget->link, &parent->children);
		widget->window = parent->window;
	} else {
		widget->parent = NULL;
		widget->window = NULL;
	}

	widget->data = data;

	widget->hpos = 0;
	widget->vpos = 0;
	widget->width = 0;
	widget->height = 0;

	widget->width_min = 0;
	widget->height_min = 0;
	widget->width_ideal = 0;
	widget->height_ideal = 0;
	widget->width_max = SIZE_MAX;
	widget->height_max = SIZE_MAX;
}

/** Change position and size of the widget. */
void widget_modify(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	widget->hpos = hpos;
	widget->vpos = vpos;
	widget->width = width;
	widget->height = height;
}

/** Get custom client data */
const void *widget_get_data(widget_t *widget)
{
	return widget->data;
}

/** Unlink widget from its parent. */
void widget_deinit(widget_t *widget)
{
	if (widget->parent)
		list_remove(&widget->link);
}

/** @}
 */

