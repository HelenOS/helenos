/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IO_CHARDEV_SRV_H
#define LIBDEVICE_IO_CHARDEV_SRV_H

#include <adt/list.h>
#include <async.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stddef.h>
#include <types/io/chardev.h>

typedef struct chardev_ops chardev_ops_t;

/** Service setup (per sevice) */
typedef struct {
	chardev_ops_t *ops;
	void *sarg;
} chardev_srvs_t;

/** Server structure (per client session) */
typedef struct {
	chardev_srvs_t *srvs;
	void *carg;
} chardev_srv_t;

struct chardev_ops {
	errno_t (*open)(chardev_srvs_t *, chardev_srv_t *);
	errno_t (*close)(chardev_srv_t *);
	errno_t (*read)(chardev_srv_t *, void *, size_t, size_t *,
	    chardev_flags_t);
	errno_t (*write)(chardev_srv_t *, const void *, size_t, size_t *);
	void (*def_handler)(chardev_srv_t *, ipc_call_t *);
};

extern void chardev_srvs_init(chardev_srvs_t *);

extern errno_t chardev_conn(ipc_call_t *, chardev_srvs_t *);

#endif

/** @}
 */
