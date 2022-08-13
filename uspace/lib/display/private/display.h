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

#ifndef _LIBDISPLAY_PRIVATE_DISPLAY_H_
#define _LIBDISPLAY_PRIVATE_DISPLAY_H_

#include <adt/list.h>
#include <async.h>
#include <fibril_synch.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Display server session structure */
struct display {
	/** Session with display server */
	async_sess_t *sess;
	/** Synchronize access to display object */
	fibril_mutex_t lock;
	/** @c true if callback handler terminated */
	bool cb_done;
	/** Signalled when cb_done or ev_pending is changed */
	fibril_condvar_t cv;
	/** Windows (of display_window_t) */
	list_t windows;
};

/** Display window structure */
struct display_window {
	/** Display associated with the window */
	display_t *display;
	/** Link to @c display->windows */
	link_t lwindows;
	/** Window ID */
	sysarg_t id;
	/** Callback functions */
	display_wnd_cb_t *cb;
	/** Argument to callback functions */
	void *cb_arg;
};

#endif

/** @}
 */
