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

/**
 * @addtogroup libdrv
 * @{
 */

/** @file
 */

#include <assert.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <ctype.h>
#include <inttypes.h>
#include <devman.h>

#include "dev_iface.h"
#include "ddf/driver.h"
#include "ddf/interrupt.h"
#include "private/driver.h"

/** Driver structure */
static const driver_t *driver;

/** Devices */
LIST_INITIALIZE(devices);
FIBRIL_MUTEX_INITIALIZE(devices_mutex);

/** Functions */
LIST_INITIALIZE(functions);
FIBRIL_MUTEX_INITIALIZE(functions_mutex);

FIBRIL_RWLOCK_INITIALIZE(stopping_lock);
static bool stopping = false;

static ddf_dev_t *create_device(void);
static void delete_device(ddf_dev_t *);
static void dev_add_ref(ddf_dev_t *);
static void dev_del_ref(ddf_dev_t *);
static void fun_add_ref(ddf_fun_t *);
static void fun_del_ref(ddf_fun_t *);
static remote_handler_t *function_get_default_handler(ddf_fun_t *);
static void *function_get_ops(ddf_fun_t *, dev_inferface_idx_t);

static void add_to_functions_list(ddf_fun_t *fun)
{
	fibril_mutex_lock(&functions_mutex);
	list_append(&fun->link, &functions);
	fibril_mutex_unlock(&functions_mutex);
}

static void remove_from_functions_list(ddf_fun_t *fun)
{
	fibril_mutex_lock(&functions_mutex);
	list_remove(&fun->link);
	fibril_mutex_unlock(&functions_mutex);
}

static ddf_dev_t *driver_get_device(devman_handle_t handle)
{
	assert(fibril_mutex_is_locked(&devices_mutex));

	list_foreach(devices, link, ddf_dev_t, dev) {
		if (dev->handle == handle)
			return dev;
	}

	return NULL;
}

static ddf_fun_t *driver_get_function(devman_handle_t handle)
{
	assert(fibril_mutex_is_locked(&functions_mutex));

	list_foreach(functions, link, ddf_fun_t, fun) {
		if (fun->handle == handle)
			return fun;
	}

	return NULL;
}

static void driver_dev_add(ipc_call_t *icall)
{
	devman_handle_t dev_handle = ipc_get_arg1(icall);
	devman_handle_t parent_fun_handle = ipc_get_arg2(icall);

	char *dev_name = NULL;
	errno_t rc = async_data_write_accept((void **) &dev_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	fibril_rwlock_read_lock(&stopping_lock);

	if (stopping) {
		fibril_rwlock_read_unlock(&stopping_lock);
		async_answer_0(icall, EIO);
		return;
	}

	ddf_dev_t *dev = create_device();
	if (!dev) {
		fibril_rwlock_read_unlock(&stopping_lock);
		free(dev_name);
		async_answer_0(icall, ENOMEM);
		return;
	}

	dev->handle = dev_handle;
	dev->name = dev_name;

	/*
	 * Currently not used, parent fun handle is stored in context
	 * of the connection to the parent device driver.
	 */
	(void) parent_fun_handle;

	errno_t res = driver->driver_ops->dev_add(dev);

	if (res != EOK) {
		fibril_rwlock_read_unlock(&stopping_lock);
		dev_del_ref(dev);
		async_answer_0(icall, res);
		return;
	}

	fibril_mutex_lock(&devices_mutex);
	list_append(&dev->link, &devices);
	fibril_mutex_unlock(&devices_mutex);
	fibril_rwlock_read_unlock(&stopping_lock);

	async_answer_0(icall, res);
}

static void driver_dev_remove(ipc_call_t *icall)
{
	devman_handle_t devh = ipc_get_arg1(icall);

	fibril_mutex_lock(&devices_mutex);
	ddf_dev_t *dev = driver_get_device(devh);
	if (dev != NULL)
		dev_add_ref(dev);
	fibril_mutex_unlock(&devices_mutex);

	if (dev == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	errno_t rc;

	if (driver->driver_ops->dev_remove != NULL)
		rc = driver->driver_ops->dev_remove(dev);
	else
		rc = ENOTSUP;

	if (rc == EOK) {
		fibril_mutex_lock(&devices_mutex);
		list_remove(&dev->link);
		fibril_mutex_unlock(&devices_mutex);
		dev_del_ref(dev);
	}

	dev_del_ref(dev);
	async_answer_0(icall, rc);
}

static void driver_dev_gone(ipc_call_t *icall)
{
	devman_handle_t devh = ipc_get_arg1(icall);

	fibril_mutex_lock(&devices_mutex);
	ddf_dev_t *dev = driver_get_device(devh);
	if (dev != NULL)
		dev_add_ref(dev);
	fibril_mutex_unlock(&devices_mutex);

	if (dev == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	errno_t rc;

	if (driver->driver_ops->dev_gone != NULL)
		rc = driver->driver_ops->dev_gone(dev);
	else
		rc = ENOTSUP;

	if (rc == EOK) {
		fibril_mutex_lock(&devices_mutex);
		list_remove(&dev->link);
		fibril_mutex_unlock(&devices_mutex);
		dev_del_ref(dev);
	}

	dev_del_ref(dev);
	async_answer_0(icall, rc);
}

static void driver_fun_online(ipc_call_t *icall)
{
	devman_handle_t funh = ipc_get_arg1(icall);

	/*
	 * Look the function up. Bump reference count so that
	 * the function continues to exist until we return
	 * from the driver.
	 */
	fibril_mutex_lock(&functions_mutex);

	ddf_fun_t *fun = driver_get_function(funh);
	if (fun != NULL)
		fun_add_ref(fun);

	fibril_mutex_unlock(&functions_mutex);

	if (fun == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	/* Call driver entry point */
	errno_t rc;

	if (driver->driver_ops->fun_online != NULL)
		rc = driver->driver_ops->fun_online(fun);
	else
		rc = ENOTSUP;

	fun_del_ref(fun);

	async_answer_0(icall, rc);
}

static void driver_fun_offline(ipc_call_t *icall)
{
	devman_handle_t funh = ipc_get_arg1(icall);

	/*
	 * Look the function up. Bump reference count so that
	 * the function continues to exist until we return
	 * from the driver.
	 */
	fibril_mutex_lock(&functions_mutex);

	ddf_fun_t *fun = driver_get_function(funh);
	if (fun != NULL)
		fun_add_ref(fun);

	fibril_mutex_unlock(&functions_mutex);

	if (fun == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	/* Call driver entry point */
	errno_t rc;

	if (driver->driver_ops->fun_offline != NULL)
		rc = driver->driver_ops->fun_offline(fun);
	else
		rc = ENOTSUP;

	async_answer_0(icall, rc);
}

static void driver_stop(ipc_call_t *icall)
{
	/* Prevent new devices from being added */
	fibril_rwlock_write_lock(&stopping_lock);
	stopping = true;

	/* Check if there are any devices */
	fibril_mutex_lock(&devices_mutex);
	if (list_first(&devices) != NULL) {
		/* Devices exist, roll back */
		fibril_mutex_unlock(&devices_mutex);
		stopping = false;
		fibril_rwlock_write_unlock(&stopping_lock);
		async_answer_0(icall, EBUSY);
		return;
	}

	fibril_rwlock_write_unlock(&stopping_lock);

	/* There should be no functions at this point */
	fibril_mutex_lock(&functions_mutex);
	assert(list_first(&functions) == NULL);
	fibril_mutex_unlock(&functions_mutex);

	/* Reply with success and terminate */
	async_answer_0(icall, EOK);
	exit(0);
}

static void driver_connection_devman(ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case DRIVER_DEV_ADD:
			driver_dev_add(&call);
			break;
		case DRIVER_DEV_REMOVE:
			driver_dev_remove(&call);
			break;
		case DRIVER_DEV_GONE:
			driver_dev_gone(&call);
			break;
		case DRIVER_FUN_ONLINE:
			driver_fun_online(&call);
			break;
		case DRIVER_FUN_OFFLINE:
			driver_fun_offline(&call);
			break;
		case DRIVER_STOP:
			driver_stop(&call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}
}

/** Generic client connection handler both for applications and drivers.
 *
 * @param drv True for driver client, false for other clients
 *            (applications, services, etc.).
 *
 */
static void driver_connection_gen(ipc_call_t *icall, bool drv)
{
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call and remember the handle of
	 * the device to which the client connected.
	 */
	devman_handle_t handle = ipc_get_arg2(icall);

	fibril_mutex_lock(&functions_mutex);
	ddf_fun_t *fun = driver_get_function(handle);
	if (fun != NULL)
		fun_add_ref(fun);
	fibril_mutex_unlock(&functions_mutex);

	if (fun == NULL) {
		printf("%s: driver_connection_gen error - no function with handle"
		    " %" PRIun " was found.\n", driver->name, handle);
		async_answer_0(icall, ENOENT);
		return;
	}

	if (fun->conn_handler != NULL) {
		/* Driver has a custom connection handler. */
		(*fun->conn_handler)(icall, (void *)fun);
		fun_del_ref(fun);
		return;
	}

	/*
	 * TODO - if the client is not a driver, check whether it is allowed to
	 * use the device.
	 */

	errno_t ret = EOK;
	/* Open device function */
	if (fun->ops != NULL && fun->ops->open != NULL)
		ret = (*fun->ops->open)(fun);

	if (ret != EOK) {
		async_answer_0(icall, ret);
		fun_del_ref(fun);
		return;
	}

	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* Close device function */
			if (fun->ops != NULL && fun->ops->close != NULL)
				(*fun->ops->close)(fun);
			async_answer_0(&call, EOK);
			fun_del_ref(fun);
			return;
		}

		/* Convert ipc interface id to interface index */

		int iface_idx = DEV_IFACE_IDX(method);

		if (!is_valid_iface_idx(iface_idx)) {
			remote_handler_t *default_handler =
			    function_get_default_handler(fun);
			if (default_handler != NULL) {
				(*default_handler)(fun, &call);
				continue;
			}

			/*
			 * Function has no such interface and
			 * default handler is not provided.
			 */
			printf("%s: driver_connection_gen error - "
			    "invalid interface id %d.",
			    driver->name, iface_idx);
			async_answer_0(&call, ENOTSUP);
			continue;
		}

		/* Calling one of the function's interfaces */

		/* Get the interface ops structure. */
		void *ops = function_get_ops(fun, iface_idx);
		if (ops == NULL) {
			printf("%s: driver_connection_gen error - ", driver->name);
			printf("Function with handle %" PRIun " has no interface "
			    "with id %d.\n", handle, iface_idx);
			async_answer_0(&call, ENOTSUP);
			continue;
		}

		/*
		 * Get the corresponding interface for remote request
		 * handling ("remote interface").
		 */
		const remote_iface_t *rem_iface = get_remote_iface(iface_idx);
		assert(rem_iface != NULL);

		/* get the method of the remote interface */
		sysarg_t iface_method_idx = ipc_get_arg1(&call);
		remote_iface_func_ptr_t iface_method_ptr =
		    get_remote_method(rem_iface, iface_method_idx);
		if (iface_method_ptr == NULL) {
			/* The interface has not such method */
			printf("%s: driver_connection_gen error - "
			    "invalid interface method.", driver->name);
			async_answer_0(&call, ENOTSUP);
			continue;
		}

		/*
		 * Call the remote interface's method, which will
		 * receive parameters from the remote client and it will
		 * pass it to the corresponding local interface method
		 * associated with the function by its driver.
		 */
		(*iface_method_ptr)(fun, ops, &call);
	}
}

static void driver_connection_driver(ipc_call_t *icall, void *arg)
{
	driver_connection_gen(icall, true);
}

static void driver_connection_client(ipc_call_t *icall, void *arg)
{
	driver_connection_gen(icall, false);
}

/** Create new device structure.
 *
 * @return		The device structure.
 */
static ddf_dev_t *create_device(void)
{
	ddf_dev_t *dev;

	dev = calloc(1, sizeof(ddf_dev_t));
	if (dev == NULL)
		return NULL;

	refcount_init(&dev->refcnt);

	return dev;
}

/** Create new function structure.
 *
 * @return		The device structure.
 */
static ddf_fun_t *create_function(void)
{
	ddf_fun_t *fun;

	fun = calloc(1, sizeof(ddf_fun_t));
	if (fun == NULL)
		return NULL;

	refcount_init(&fun->refcnt);
	init_match_ids(&fun->match_ids);
	link_initialize(&fun->link);

	return fun;
}

/** Delete device structure.
 *
 * @param dev		The device structure.
 */
static void delete_device(ddf_dev_t *dev)
{
	if (dev->parent_sess)
		async_hangup(dev->parent_sess);
	if (dev->driver_data != NULL)
		free(dev->driver_data);
	if (dev->name)
		free(dev->name);
	free(dev);
}

/** Delete function structure.
 *
 * @param dev		The device structure.
 */
static void delete_function(ddf_fun_t *fun)
{
	clean_match_ids(&fun->match_ids);
	if (fun->driver_data != NULL)
		free(fun->driver_data);
	if (fun->name != NULL)
		free(fun->name);
	free(fun);
}

/** Increase device reference count. */
static void dev_add_ref(ddf_dev_t *dev)
{
	refcount_up(&dev->refcnt);
}

/** Decrease device reference count.
 *
 * Free the device structure if the reference count drops to zero.
 */
static void dev_del_ref(ddf_dev_t *dev)
{
	if (refcount_down(&dev->refcnt))
		delete_device(dev);
}

/** Increase function reference count.
 *
 * This also increases reference count on the device. The device structure
 * will thus not be deallocated while there are some associated function
 * structures.
 */
static void fun_add_ref(ddf_fun_t *fun)
{
	dev_add_ref(fun->dev);
	refcount_up(&fun->refcnt);
}

/** Decrease function reference count.
 *
 * Free the function structure if the reference count drops to zero.
 */
static void fun_del_ref(ddf_fun_t *fun)
{
	ddf_dev_t *dev = fun->dev;

	if (refcount_down(&fun->refcnt))
		delete_function(fun);

	dev_del_ref(dev);
}

/** Allocate driver-specific device data. */
void *ddf_dev_data_alloc(ddf_dev_t *dev, size_t size)
{
	assert(dev->driver_data == NULL);

	void *data = calloc(1, size);
	if (data == NULL)
		return NULL;

	dev->driver_data = data;
	return data;
}

/** Return driver-specific device data. */
void *ddf_dev_data_get(ddf_dev_t *dev)
{
	return dev->driver_data;
}

/** Get device handle. */
devman_handle_t ddf_dev_get_handle(ddf_dev_t *dev)
{
	return dev->handle;
}

/** Return device name.
 *
 * @param dev	Device
 * @return	Device name. Valid as long as @a dev is valid.
 */
const char *ddf_dev_get_name(ddf_dev_t *dev)
{
	return dev->name;
}

/** Return existing session with the parent function.
 *
 * @param dev	Device
 * @return	Session with parent function or NULL upon failure
 */
async_sess_t *ddf_dev_parent_sess_get(ddf_dev_t *dev)
{
	if (dev->parent_sess == NULL) {
		dev->parent_sess = devman_parent_device_connect(dev->handle,
		    IPC_FLAG_BLOCKING);
	}

	return dev->parent_sess;
}

/** Set function name (if it was not specified when node was created.)
 *
 * @param dev	Device whose name has not been set yet
 * @param name	Name, will be copied
 * @return	EOK on success, ENOMEM if out of memory
 */
errno_t ddf_fun_set_name(ddf_fun_t *dev, const char *name)
{
	assert(dev->name == NULL);

	dev->name = str_dup(name);
	if (dev->name == NULL)
		return ENOENT;

	return EOK;
}

/** Get device to which function belongs. */
ddf_dev_t *ddf_fun_get_dev(ddf_fun_t *fun)
{
	return fun->dev;
}

/** Get function handle.
 *
 * XXX USB uses this, but its use should be eliminated.
 */
devman_handle_t ddf_fun_get_handle(ddf_fun_t *fun)
{
	return fun->handle;
}

/** Create a DDF function node.
 *
 * Create a DDF function (in memory). Both child devices and external clients
 * communicate with a device via its functions.
 *
 * The created function node is fully formed, but only exists in the memory
 * of the client task. In order to be visible to the system, the function
 * must be bound using ddf_fun_bind().
 *
 * This function should only fail if there is not enough free memory.
 * Specifically, this function succeeds even if @a dev already has
 * a (bound) function with the same name. @a name can be NULL in which
 * case the caller will set the name later using ddf_fun_set_name().
 * He must do this before binding the function.
 *
 * Type: A function of type fun_inner indicates that DDF should attempt
 * to attach child devices to the function. fun_exposed means that
 * the function should be exported to external clients (applications).
 *
 * @param dev		Device to which we are adding function
 * @param ftype		Type of function (fun_inner or fun_exposed)
 * @param name		Name of function or NULL
 *
 * @return		New function or @c NULL if memory is not available
 */
ddf_fun_t *ddf_fun_create(ddf_dev_t *dev, fun_type_t ftype, const char *name)
{
	ddf_fun_t *fun = create_function();
	if (fun == NULL)
		return NULL;

	fun->dev = dev;
	dev_add_ref(fun->dev);

	fun->bound = false;
	fun->ftype = ftype;

	if (name != NULL) {
		fun->name = str_dup(name);
		if (fun->name == NULL) {
			fun_del_ref(fun);	/* fun is destroyed */
			return NULL;
		}
	}

	return fun;
}

/** Allocate driver-specific function data. */
void *ddf_fun_data_alloc(ddf_fun_t *fun, size_t size)
{
	assert(fun->bound == false);
	assert(fun->driver_data == NULL);

	void *data = calloc(1, size);
	if (data == NULL)
		return NULL;

	fun->driver_data = data;
	return data;
}

/** Return driver-specific function data. */
void *ddf_fun_data_get(ddf_fun_t *fun)
{
	return fun->driver_data;
}

/** Return function name.
 *
 * @param fun	Function
 * @return	Function name. Valid as long as @a fun is valid.
 */
const char *ddf_fun_get_name(ddf_fun_t *fun)
{
	return fun->name;
}

/** Destroy DDF function node.
 *
 * Destroy a function previously created with ddf_fun_create(). The function
 * must not be bound.
 *
 * @param fun Function to destroy
 *
 */
void ddf_fun_destroy(ddf_fun_t *fun)
{
	assert(fun->bound == false);

	/*
	 * Drop the reference added by ddf_fun_create(). This will deallocate
	 * the function as soon as all other references are dropped (i.e.
	 * as soon control leaves all driver entry points called in context
	 * of this function.
	 */
	fun_del_ref(fun);
}

static void *function_get_ops(ddf_fun_t *fun, dev_inferface_idx_t idx)
{
	assert(is_valid_iface_idx(idx));
	if (fun->ops == NULL)
		return NULL;

	return fun->ops->interfaces[idx];
}

/** Bind a function node.
 *
 * Bind the specified function to the system. This effectively makes
 * the function visible to the system (uploads it to the server).
 *
 * This function can fail for several reasons. Specifically,
 * it will fail if the device already has a bound function of
 * the same name.
 *
 * @param fun Function to bind
 *
 * @return EOK on success or an error code
 *
 */
errno_t ddf_fun_bind(ddf_fun_t *fun)
{
	assert(fun->bound == false);
	assert(fun->name != NULL);
	assert(fun->dev != NULL);

	add_to_functions_list(fun);
	errno_t res = devman_add_function(fun->name, fun->ftype, &fun->match_ids,
	    fun->dev->handle, &fun->handle);
	if (res != EOK) {
		remove_from_functions_list(fun);
		return res;
	}

	fun->bound = true;
	return res;
}

/** Unbind a function node.
 *
 * Unbind the specified function from the system. This effectively makes
 * the function invisible to the system.
 *
 * @param fun Function to unbind
 *
 * @return EOK on success or an error code
 *
 */
errno_t ddf_fun_unbind(ddf_fun_t *fun)
{
	assert(fun->bound == true);

	errno_t res = devman_remove_function(fun->handle);
	if (res != EOK)
		return res;

	remove_from_functions_list(fun);

	fun->bound = false;
	return EOK;
}

/** Online function.
 *
 * @param fun Function to online
 *
 * @return EOK on success or an error code
 *
 */
errno_t ddf_fun_online(ddf_fun_t *fun)
{
	assert(fun->bound == true);

	errno_t res = devman_drv_fun_online(fun->handle);
	if (res != EOK)
		return res;

	return EOK;
}

/** Offline function.
 *
 * @param fun Function to offline
 *
 * @return EOK on success or an error code
 *
 */
errno_t ddf_fun_offline(ddf_fun_t *fun)
{
	assert(fun->bound == true);

	errno_t res = devman_drv_fun_offline(fun->handle);
	if (res != EOK)
		return res;

	return EOK;
}

/** Add single match ID to inner function.
 *
 * Construct and add a single match ID to the specified function.
 * Cannot be called when the function node is bound.
 *
 * @param fun          Function
 * @param match_id_str Match string
 * @param match_score  Match score
 *
 * @return EOK on success.
 * @return ENOMEM if out of memory.
 *
 */
errno_t ddf_fun_add_match_id(ddf_fun_t *fun, const char *match_id_str,
    int match_score)
{
	assert(fun->bound == false);
	assert(fun->ftype == fun_inner);

	match_id_t *match_id = create_match_id();
	if (match_id == NULL)
		return ENOMEM;

	match_id->id = str_dup(match_id_str);
	match_id->score = match_score;

	add_match_id(&fun->match_ids, match_id);
	return EOK;
}

/** Set function ops. */
void ddf_fun_set_ops(ddf_fun_t *fun, const ddf_dev_ops_t *dev_ops)
{
	assert(fun->conn_handler == NULL);
	fun->ops = dev_ops;
}

/** Set user-defined connection handler.
 *
 * This allows handling connections the non-devman way.
 */
void ddf_fun_set_conn_handler(ddf_fun_t *fun, async_port_handler_t conn)
{
	assert(fun->ops == NULL);
	fun->conn_handler = conn;
}

/** Get default handler for client requests */
static remote_handler_t *function_get_default_handler(ddf_fun_t *fun)
{
	if (fun->ops == NULL)
		return NULL;
	return fun->ops->default_handler;
}

/** Add exposed function to category.
 *
 * Must only be called when the function is bound.
 *
 */
errno_t ddf_fun_add_to_category(ddf_fun_t *fun, const char *cat_name)
{
	assert(fun->bound == true);
	assert(fun->ftype == fun_exposed);

	return devman_add_device_to_category(fun->handle, cat_name);
}

errno_t ddf_driver_main(const driver_t *drv)
{
	/*
	 * Remember the driver structure - driver_ops will be called by generic
	 * handler for incoming connections.
	 */
	driver = drv;

	/*
	 * Register driver with device manager using generic handler for
	 * incoming connections.
	 */
	port_id_t port;
	errno_t rc = async_create_port(INTERFACE_DDF_DRIVER, driver_connection_driver,
	    NULL, &port);
	if (rc != EOK) {
		printf("Error: Failed to create driver port.\n");
		return rc;
	}

	rc = async_create_port(INTERFACE_DDF_DEVMAN, driver_connection_devman,
	    NULL, &port);
	if (rc != EOK) {
		printf("Error: Failed to create devman port.\n");
		return rc;
	}

	async_set_fallback_port_handler(driver_connection_client, NULL);

	rc = devman_driver_register(driver->name);
	if (rc != EOK) {
		printf("Error: Failed to register driver with device manager "
		    "(%s).\n", (rc == EEXIST) ? "driver already started" :
		    str_error(rc));

		return rc;
	}

	/* Return success from the task since server has started. */
	rc = task_retval(0);
	if (rc != EOK) {
		printf("Error: Failed returning task value.\n");
		return rc;
	}

	async_manager();

	/* Never reached. */
	return EOK;
}

/**
 * @}
 */
