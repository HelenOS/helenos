/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file Display server window type
 */

#ifndef TYPES_DISPLAY_WINDOW_H
#define TYPES_DISPLAY_WINDOW_H

#include <adt/list.h>
#include <gfx/context.h>
#include <gfx/coord.h>

typedef sysarg_t ds_wnd_id_t;

/** Display server window */
typedef struct ds_window {
	/** Parent display */
	struct ds_display *display;
	/** Link to @c display->windows */
	link_t lwindows;
	/** Display position */
	gfx_coord2_t dpos;
	/** Window ID */
	ds_wnd_id_t id;
	/** Graphic context */
	gfx_context_t *gc;
} ds_window_t;

/** Bitmap in display server window GC */
typedef struct {
	/** Containing window */
	ds_window_t *wnd;
	/** Display bitmap */
	gfx_bitmap_t *bitmap;
} ds_window_bitmap_t;

#endif

/** @}
 */
