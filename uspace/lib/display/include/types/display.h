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

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_TYPES_DISPLAY_H_
#define _LIBDISPLAY_TYPES_DISPLAY_H_

#include <async.h>
#include <fibril_synch.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stddef.h>
#include <stdint.h>

/** Use the default display service (argument to display_open() */
#define DISPLAY_DEFAULT NULL

struct display;
struct display_window;

/** Display server session */
typedef struct display display_t;

/** Display window */
typedef struct display_window display_window_t;

/** Display window callbacks */
typedef struct {
	/** Close event */
	void (*close_event)(void *);
	/** Focus event */
	void (*focus_event)(void *, unsigned);
	/** Keyboard event */
	void (*kbd_event)(void *, kbd_event_t *);
	/** Position event */
	void (*pos_event)(void *, pos_event_t *);
	/** Resize event */
	void (*resize_event)(void *, gfx_rect_t *);
	/** Unfocus event */
	void (*unfocus_event)(void *, unsigned);
} display_wnd_cb_t;

#endif

/** @}
 */
