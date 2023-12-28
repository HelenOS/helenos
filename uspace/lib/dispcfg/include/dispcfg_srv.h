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

/** @addtogroup libdispcfg
 * @{
 */
/** @file
 */

#ifndef _LIBDISPCFG_DISPCFG_SRV_H_
#define _LIBDISPCFG_DISPCFG_SRV_H_

#include <async.h>
#include <errno.h>
#include "types/dispcfg.h"

typedef struct dispcfg_ops dispcfg_ops_t;

/** Display configuration server structure (per client session) */
typedef struct {
	async_sess_t *client_sess;
	dispcfg_ops_t *ops;
	void *arg;
} dispcfg_srv_t;

struct dispcfg_ops {
	errno_t (*get_seat_list)(void *, dispcfg_seat_list_t **);
	errno_t (*get_seat_info)(void *, sysarg_t, dispcfg_seat_info_t **);
	errno_t (*seat_create)(void *, const char *, sysarg_t *);
	errno_t (*seat_delete)(void *, sysarg_t);
	errno_t (*dev_assign)(void *, sysarg_t, sysarg_t);
	errno_t (*dev_unassign)(void *, sysarg_t);
	errno_t (*get_asgn_dev_list)(void *, sysarg_t, dispcfg_dev_list_t **);
	errno_t (*get_event)(void *, dispcfg_ev_t *);
};

extern void dispcfg_conn(ipc_call_t *, dispcfg_srv_t *);
extern void dispcfg_srv_initialize(dispcfg_srv_t *);
extern void dispcfg_srv_ev_pending(dispcfg_srv_t *);

#endif

/** @}
 */
