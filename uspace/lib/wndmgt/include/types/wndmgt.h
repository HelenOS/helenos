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

/** @addtogroup libwndmgt
 * @{
 */
/** @file
 */

#ifndef _LIBWNDMGT_TYPES_WNDMGT_H_
#define _LIBWNDMGT_TYPES_WNDMGT_H_

#include <ipc/services.h>
#include <types/common.h>

/** Use the default window management service (argument to wndmgt_open() */
#define WNDMGT_DEFAULT SERVICE_NAME_WNDMGT

struct wndmgt;

/** Window management session */
typedef struct wndmgt wndmgt_t;

/** Window management callbacks */
typedef struct {
	/** Window added */
	void (*window_added)(void *, sysarg_t);
	/** Window removed */
	void (*window_removed)(void *, sysarg_t);
	/** Window changed */
	void (*window_changed)(void *, sysarg_t);
} wndmgt_cb_t;

/** Window management event type */
typedef enum {
	/** Window added */
	wmev_window_added,
	/** Window removed */
	wmev_window_removed,
	/** Window changed */
	wmev_window_changed
} wndmgt_ev_type_t;

/** Window management event */
typedef struct {
	/** Event type */
	wndmgt_ev_type_t etype;
	/** Window ID */
	sysarg_t wnd_id;
} wndmgt_ev_t;

/** Window list */
typedef struct {
	/** Number of windows */
	size_t nwindows;
	/** ID for each window */
	sysarg_t *windows;
} wndmgt_window_list_t;

/** Window information */
typedef struct {
	/** Window caption */
	char *caption;
	/** Window flags */
	unsigned flags;
	/** Number of foci on this window */
	unsigned nfocus;
} wndmgt_window_info_t;

#endif

/** @}
 */
