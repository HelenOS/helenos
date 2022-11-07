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

/** @addtogroup libwndmgt
 * @{
 */
/** @file
 */

#ifndef _LIBWNDMGT_WNDMGT_SRV_H_
#define _LIBWNDMGT_WNDMGT_SRV_H_

#include <async.h>
#include <errno.h>
#include "types/wndmgt.h"

typedef struct wndmgt_ops wndmgt_ops_t;

/** Window management server structure (per client session) */
typedef struct {
	async_sess_t *client_sess;
	wndmgt_ops_t *ops;
	void *arg;
} wndmgt_srv_t;

struct wndmgt_ops {
	errno_t (*get_window_list)(void *, wndmgt_window_list_t **);
	errno_t (*get_window_info)(void *, sysarg_t, wndmgt_window_info_t **);
	errno_t (*activate_window)(void *, sysarg_t, sysarg_t);
	errno_t (*close_window)(void *, sysarg_t);
	errno_t (*get_event)(void *, wndmgt_ev_t *);
};

extern void wndmgt_conn(ipc_call_t *, wndmgt_srv_t *);
extern void wndmgt_srv_initialize(wndmgt_srv_t *);
extern void wndmgt_srv_ev_pending(wndmgt_srv_t *);

#endif

/** @}
 */
