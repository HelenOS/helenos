/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/*
 * Device
 */

typedef struct ddf_dev ddf_dev_t;
typedef struct ddf_fun ddf_fun_t;

/** Devices operations */
typedef struct ddf_dev_ops {
	/**
	 * Optional callback function called when a client is connecting to the
	 * device.
	 */
	errno_t (*open)(ddf_fun_t *);

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
struct ddf_dev;

/** Function structure */
struct ddf_fun;

/*
 * Driver
 */

/** Generic device driver operations */
typedef struct driver_ops {
	/** Callback method for passing a new device to the device driver */
	errno_t (*dev_add)(ddf_dev_t *);

	/** Ask driver to remove a device */
	errno_t (*dev_remove)(ddf_dev_t *);

	/** Inform driver a device disappeared */
	errno_t (*dev_gone)(ddf_dev_t *);

	/** Ask driver to online a specific function */
	errno_t (*fun_online)(ddf_fun_t *);

	/** Ask driver to offline a specific function */
	errno_t (*fun_offline)(ddf_fun_t *);
} driver_ops_t;

/** Driver structure */
typedef struct driver {
	/** Name of the device driver */
	const char *name;
	/** Generic device driver operations */
	const driver_ops_t *driver_ops;
} driver_t;

extern int ddf_driver_main(const driver_t *);

extern void *ddf_dev_data_alloc(ddf_dev_t *, size_t);
extern void *ddf_dev_data_get(ddf_dev_t *);
extern devman_handle_t ddf_dev_get_handle(ddf_dev_t *);
extern const char *ddf_dev_get_name(ddf_dev_t *);
extern async_sess_t *ddf_dev_parent_sess_get(ddf_dev_t *);
extern ddf_fun_t *ddf_fun_create(ddf_dev_t *, fun_type_t, const char *);
extern devman_handle_t ddf_fun_get_handle(ddf_fun_t *);
extern void ddf_fun_destroy(ddf_fun_t *);
extern void *ddf_fun_data_alloc(ddf_fun_t *, size_t);
extern void *ddf_fun_data_get(ddf_fun_t *);
extern const char *ddf_fun_get_name(ddf_fun_t *);
extern errno_t ddf_fun_set_name(ddf_fun_t *, const char *);
extern ddf_dev_t *ddf_fun_get_dev(ddf_fun_t *);
extern errno_t ddf_fun_bind(ddf_fun_t *);
extern errno_t ddf_fun_unbind(ddf_fun_t *);
extern errno_t ddf_fun_online(ddf_fun_t *);
extern errno_t ddf_fun_offline(ddf_fun_t *);
extern errno_t ddf_fun_add_match_id(ddf_fun_t *, const char *, int);
extern void ddf_fun_set_ops(ddf_fun_t *, const ddf_dev_ops_t *);
extern void ddf_fun_set_conn_handler(ddf_fun_t *, async_port_handler_t);
extern errno_t ddf_fun_add_to_category(ddf_fun_t *, const char *);

#endif

/**
 * @}
 */
