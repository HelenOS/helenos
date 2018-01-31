/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef DDF_PRIVATE_DRIVER_H_
#define DDF_PRIVATE_DRIVER_H_

#include <async.h>
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
	atomic_t refcnt;
	
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
	atomic_t refcnt;
	
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
