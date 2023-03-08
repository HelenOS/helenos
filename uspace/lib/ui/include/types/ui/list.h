/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file List control
 */

#ifndef _UI_TYPES_LIST_H
#define _UI_TYPES_LIST_H

#include <gfx/color.h>

struct ui_list;
typedef struct ui_list ui_list_t;

struct ui_list_entry;
typedef struct ui_list_entry ui_list_entry_t;

/** List entry attributes */
typedef struct ui_list_entry_attr {
	/** Entry caption */
	const char *caption;
	/** User argument */
	void *arg;
	/** Entry color or @c NULL to use default color */
	gfx_color_t *color;
	/** Entry background color or @c NULL to use default color */
	gfx_color_t *bgcolor;
} ui_list_entry_attr_t;

/** List callbacks */
typedef struct ui_list_cb {
	/** Request list activation */
	void (*activate_req)(ui_list_t *, void *);
	/** Entry was selected */
	void (*selected)(ui_list_entry_t *, void *);
	/** Compare two entries (for sorting) */
	int (*compare)(ui_list_entry_t *, ui_list_entry_t *);
} ui_list_cb_t;

#endif

/** @}
 */
