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

#ifndef _LIBDDEV_DDEV_SRV_H_
#define _LIBDDEV_DDEV_SRV_H_

#include <async.h>
#include <errno.h>
#include <gfx/context.h>
#include "types/ddev/info.h"

typedef struct ddev_ops ddev_ops_t;

/** Display device server structure (per client session) */
typedef struct {
	async_sess_t *client_sess;
	ddev_ops_t *ops;
	void *arg;
} ddev_srv_t;

struct ddev_ops {
	errno_t (*get_gc)(void *, sysarg_t *, sysarg_t *);
	errno_t (*get_info)(void *, ddev_info_t *);
};

extern void ddev_conn(ipc_call_t *, ddev_srv_t *);
extern void ddev_srv_initialize(ddev_srv_t *);

#endif

/** @}
 */
