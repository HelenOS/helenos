/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_DISP_SRV_H_
#define _LIBDISPLAY_DISP_SRV_H_

#include <async.h>
#include <errno.h>
#include <gfx/coord.h>
#include "display/wndparams.h"
#include "types/display/cursor.h"
#include "types/display/event.h"
#include "types/display/info.h"
#include "types/display/wndresize.h"

typedef struct display_ops display_ops_t;

/** Display server structure (per client session) */
typedef struct {
	async_sess_t *client_sess;
	display_ops_t *ops;
	void *arg;
} display_srv_t;

struct display_ops {
	errno_t (*window_create)(void *, display_wnd_params_t *, sysarg_t *);
	errno_t (*window_destroy)(void *, sysarg_t);
	errno_t (*window_move_req)(void *, sysarg_t, gfx_coord2_t *);
	errno_t (*window_resize_req)(void *, sysarg_t, display_wnd_rsztype_t,
	    gfx_coord2_t *);
	errno_t (*window_move)(void *, sysarg_t, gfx_coord2_t *);
	errno_t (*window_get_pos)(void *, sysarg_t, gfx_coord2_t *);
	errno_t (*window_get_max_rect)(void *, sysarg_t, gfx_rect_t *);
	errno_t (*window_resize)(void *, sysarg_t, gfx_coord2_t *, gfx_rect_t *);
	errno_t (*window_maximize)(void *, sysarg_t);
	errno_t (*window_unmaximize)(void *, sysarg_t);
	errno_t (*window_set_cursor)(void *, sysarg_t, display_stock_cursor_t);
	errno_t (*get_event)(void *, sysarg_t *, display_wnd_ev_t *);
	errno_t (*get_info)(void *, display_info_t *);
};

extern void display_conn(ipc_call_t *, display_srv_t *);
extern void display_srv_initialize(display_srv_t *);
extern void display_srv_ev_pending(display_srv_t *);

#endif

/** @}
 */
