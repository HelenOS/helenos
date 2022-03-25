/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file Scrollbar
 */

#ifndef _UI_TYPES_SCROLLBAR_H
#define _UI_TYPES_SCROLLBAR_H

#include <gfx/coord.h>

struct ui_scrollbar;
typedef struct ui_scrollbar ui_scrollbar_t;

/** Scrollbar direction */
typedef enum {
	/** Horizontal */
	ui_sbd_horiz,
	/** Vertical */
	ui_sbd_vert
} ui_scrollbar_dir_t;

/** Scrollbar callbacks */
typedef struct ui_scrollbar_cb {
	void (*up)(ui_scrollbar_t *, void *);
	void (*down)(ui_scrollbar_t *, void *);
	void (*page_up)(ui_scrollbar_t *, void *);
	void (*page_down)(ui_scrollbar_t *, void *);
	void (*moved)(ui_scrollbar_t *, void *, gfx_coord_t);
} ui_scrollbar_cb_t;

#endif

/** @}
 */
