/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef DDF_PRIVATE_DRIVER_H_
#define DDF_PRIVATE_DRIVER_H_

#include <async.h>
#include <refcount.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include "dev_iface.h"

/** Device structure */
struct ddf_dev {
	/**
	 * Globally unique device identifier (assigned to the device by the
	 * device manager).
	 */
	devman_handle_t handle;

	/** Reference count */
	atomic_refcount_t refcnt;

	/** Session with the parent device driver */
	async_sess_t *parent_sess;

	/** Device name */
	char *name;

	/** Driver-specific data associated with this device */
	void *driver_data;

	/** Link in the list of devices handled by the driver */
	link_t link;
};

/** Function structure */
struct ddf_fun {
	/** True if bound to the device manager */
	bool bound;

	/** Function indentifier (asigned by device manager) */
	devman_handle_t handle;

	/** Reference count */
	atomic_refcount_t refcnt;

	/** Device which this function belogs to */
	struct ddf_dev *dev;

	/** Function type */
	fun_type_t ftype;

	/** Function name */
	char *name;

	/** List of device ids for driver matching */
	match_id_list_t match_ids;

	/** Driver-specific data associated with this function */
	void *driver_data;

	/** Implementation of operations provided by this function */
	const ddf_dev_ops_t *ops;

	/** Connection handler or @c NULL to use the DDF default handler. */
	async_port_handler_t conn_handler;

	/** Link in the list of functions handled by the driver */
	link_t link;
};

#endif

/**
 * @}
 */
