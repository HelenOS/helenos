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
