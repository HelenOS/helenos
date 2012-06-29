/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jiri Svoboda
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

#ifndef DDF_DRIVER_H_
#define DDF_DRIVER_H_

#include <async.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include "../dev_iface.h"

typedef struct ddf_dev ddf_dev_t;
typedef struct ddf_fun ddf_fun_t;

/*
 * Device
 */

/** Devices operations */
typedef struct ddf_dev_ops {
	/**
	 * Optional callback function called when a client is connecting to the
	 * device.
	 */
	int (*open)(ddf_fun_t *);
	
	/**
	 * Optional callback function called when a client is disconnecting from
	 * the device.
	 */
	void (*close)(ddf_fun_t *);
	
	/** The table of standard interfaces implemented by the device. */
	void *interfaces[DEV_IFACE_COUNT];
	
	/**
	 * The default handler of remote client requests. If the client's remote
	 * request cannot be handled by any of the standard interfaces, the
	 * default handler is used.
	 */
	remote_handler_t *default_handler;
} ddf_dev_ops_t;

/** Device structure */
struct ddf_dev {
	/**
	 * Globally unique device identifier (assigned to the device by the
	 * device manager).
	 */
	devman_handle_t handle;
	
	/** Reference count */
	atomic_t refcnt;
	
	/**
	 * Session to the parent device driver (if it is different from this
	 * driver)
	 */
	async_sess_t *parent_sess;
	
	/** Device name */
	const char *name;
	
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
	ddf_dev_t *dev;
	
	/** Function type */
	fun_type_t ftype;
	
	/** Function name */
	const char *name;
	
	/** List of device ids for driver matching */
	match_id_list_t match_ids;
	
	/** Driver-specific data associated with this function */
	void *driver_data;
	
	/** Implementation of operations provided by this function */
	ddf_dev_ops_t *ops;
	
	/** Connection handler or @c NULL to use the DDF default handler. */
	async_client_conn_t conn_handler;
	
	/** Link in the list of functions handled by the driver */
	link_t link;
};

/*
 * Driver
 */

/** Generic device driver operations */
typedef struct driver_ops {
	/** Callback method for passing a new device to the device driver */
	int (*dev_add)(ddf_dev_t *);
	
	/** Ask driver to remove a device */
	int (*dev_remove)(ddf_dev_t *);
	
	/** Inform driver a device disappeared */
	int (*dev_gone)(ddf_dev_t *);
	
	/** Ask driver to online a specific function */
	int (*fun_online)(ddf_fun_t *);
	
	/** Ask driver to offline a specific function */
	int (*fun_offline)(ddf_fun_t *);
} driver_ops_t;

/** Driver structure */
typedef struct driver {
	/** Name of the device driver */
	const char *name;
	/** Generic device driver operations */
	driver_ops_t *driver_ops;
} driver_t;

extern int ddf_driver_main(driver_t *);

extern void *ddf_dev_data_alloc(ddf_dev_t *, size_t);
extern ddf_fun_t *ddf_fun_create(ddf_dev_t *, fun_type_t, const char *);
extern void ddf_fun_destroy(ddf_fun_t *);
extern void *ddf_fun_data_alloc(ddf_fun_t *, size_t);
extern int ddf_fun_bind(ddf_fun_t *);
extern int ddf_fun_unbind(ddf_fun_t *);
extern int ddf_fun_online(ddf_fun_t *);
extern int ddf_fun_offline(ddf_fun_t *);
extern int ddf_fun_add_match_id(ddf_fun_t *, const char *, int);

extern int ddf_fun_add_to_category(ddf_fun_t *, const char *);

#endif

/**
 * @}
 */
