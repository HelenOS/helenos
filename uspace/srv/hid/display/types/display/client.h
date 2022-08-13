/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server client type
 */

#ifndef TYPES_DISPLAY_CLIENT_H
#define TYPES_DISPLAY_CLIENT_H

#include <adt/list.h>

typedef sysarg_t ds_wnd_id_t;

/** Display server client callbacks */
typedef struct {
	void (*ev_pending)(void *);
} ds_client_cb_t;

/** Display server client */
typedef struct ds_client {
	/** Parent display */
	struct ds_display *display;
	/** Callbacks */
	ds_client_cb_t *cb;
	/** Callback argument */
	void *cb_arg;
	/** Link to @c display->clients */
	link_t lclients;
	/** Windows (of ds_window_t) */
	list_t windows;
	/** Event queue (of ds_window_ev_t) */
	list_t events;
} ds_client_t;

#endif

/** @}
 */
