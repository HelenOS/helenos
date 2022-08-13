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
