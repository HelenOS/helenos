/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_BD_SRV_H
#define LIBDEVICE_BD_SRV_H

#include <adt/list.h>
#include <async.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <offset.h>

typedef struct bd_ops bd_ops_t;

/** Service setup (per sevice) */
typedef struct {
	bd_ops_t *ops;
	void *sarg;
} bd_srvs_t;

/** Server structure (per client session) */
typedef struct {
	bd_srvs_t *srvs;
	async_sess_t *client_sess;
	void *carg;
} bd_srv_t;

struct bd_ops {
	errno_t (*open)(bd_srvs_t *, bd_srv_t *);
	errno_t (*close)(bd_srv_t *);
	errno_t (*read_blocks)(bd_srv_t *, aoff64_t, size_t, void *, size_t);
	errno_t (*read_toc)(bd_srv_t *, uint8_t, void *, size_t);
	errno_t (*sync_cache)(bd_srv_t *, aoff64_t, size_t);
	errno_t (*write_blocks)(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
	errno_t (*get_block_size)(bd_srv_t *, size_t *);
	errno_t (*get_num_blocks)(bd_srv_t *, aoff64_t *);
};

extern void bd_srvs_init(bd_srvs_t *);

extern errno_t bd_conn(ipc_call_t *, bd_srvs_t *);

#endif

/** @}
 */
