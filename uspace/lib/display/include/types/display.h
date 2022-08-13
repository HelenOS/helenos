/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	void (*focus_event)(void *);
	/** Keyboard event */
	void (*kbd_event)(void *, kbd_event_t *);
	/** Position event */
	void (*pos_event)(void *, pos_event_t *);
	/** Resize event */
	void (*resize_event)(void *, gfx_rect_t *);
	/** Unfocus event */
	void (*unfocus_event)(void *);
} display_wnd_cb_t;

#endif

/** @}
 */
