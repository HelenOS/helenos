/*
 * Copyright (c) 2010 Lenka Trochtova
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
 * @defgroup devman Device manager.
 * @brief HelenOS device manager.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <io/log.h>
#include <ipc/devman.h>
#include <loc.h>

#include "client_conn.h"
#include "dev.h"
#include "devman.h"
#include "devtree.h"
#include "driver.h"
#include "drv_conn.h"
#include "fun.h"
#include "loc.h"
#include "main.h"

static errno_t init_running_drv(void *drv);

/** Register running driver. */
static driver_t *devman_driver_register(cap_call_handle_t chandle, ipc_call_t *call)
{
	driver_t *driver = NULL;
	char *drv_name = NULL;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_driver_register");

	/* Get driver name. */
	errno_t rc = async_data_write_accept((void **) &drv_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		return NULL;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "The `%s' driver is trying to register.",
	    drv_name);

	/* Find driver structure. */
	driver = driver_find_by_name(&drivers_list, drv_name);
	if (driver == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "No driver named `%s' was found.", drv_name);
		free(drv_name);
		drv_name = NULL;
		async_answer_0(chandle, ENOENT);
		return NULL;
	}

	free(drv_name);
	drv_name = NULL;

	fibril_mutex_lock(&driver->driver_mutex);

	if (driver->sess) {
		/* We already have a connection to the driver. */
		log_msg(LOG_DEFAULT, LVL_ERROR, "Driver '%s' already started.\n",
		    driver->name);
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(chandle, EEXIST);
		return NULL;
	}

	switch (driver->state) {
	case DRIVER_NOT_STARTED:
		/* Somebody started the driver manually. */
		log_msg(LOG_DEFAULT, LVL_NOTE, "Driver '%s' started manually.\n",
		    driver->name);
		driver->state = DRIVER_STARTING;
		break;
	case DRIVER_STARTING:
		/* The expected case */
		break;
	case DRIVER_RUNNING:
		/* Should not happen since we do not have a connected session */
		assert(false);
	}

	/* Create connection to the driver. */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "Creating connection to the `%s' driver.",
	    driver->name);
	driver->sess = async_callback_receive(EXCHANGE_PARALLEL);
	if (!driver->sess) {
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(chandle, ENOTSUP);
		return NULL;
	}
	/* FIXME: Work around problem with callback sessions */
	async_sess_args_set(driver->sess, INTERFACE_DDF_DEVMAN, 0, 0);

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "The `%s' driver was successfully registered as running.",
	    driver->name);

	/*
	 * Initialize the driver as running (e.g. pass assigned devices to it)
	 * in a separate fibril; the separate fibril is used to enable the
	 * driver to use devman service during the driver's initialization.
	 */
	fid_t fid = fibril_create(init_running_drv, driver);
	if (fid == 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to create initialization fibril "
		    "for driver `%s'.", driver->name);
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(chandle, ENOMEM);
		return NULL;
	}

	fibril_add_ready(fid);
	fibril_mutex_unlock(&driver->driver_mutex);

	async_answer_0(chandle, EOK);
	return driver;
}

/** Receive device match ID from the device's parent driver and add it to the
 * list of devices match ids.
 *
 * @param match_ids	The list of the device's match ids.
 * @return		Zero on success, error code otherwise.
 */
static errno_t devman_receive_match_id(match_id_list_t *match_ids)
{
	match_id_t *match_id = create_match_id();
	cap_call_handle_t chandle;
	ipc_call_t call;
	errno_t rc = 0;

	chandle = async_get_call(&call);
	if (DEVMAN_ADD_MATCH_ID != IPC_GET_IMETHOD(call)) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Invalid protocol when trying to receive match id.");
		async_answer_0(chandle, EINVAL);
		delete_match_id(match_id);
		return EINVAL;
	}

	if (match_id == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to allocate match id.");
		async_answer_0(chandle, ENOMEM);
		return ENOMEM;
	}

	async_answer_0(chandle, EOK);

	match_id->score = IPC_GET_ARG1(call);

	char *match_id_str;
	rc = async_data_write_accept((void **) &match_id_str, true, 0, 0, 0, 0);
	match_id->id = match_id_str;
	if (rc != EOK) {
		delete_match_id(match_id);
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to receive match id string: %s.",
		    str_error(rc));
		return rc;
	}

	list_append(&match_id->link, &match_ids->ids);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Received match id `%s', score %d.",
	    match_id->id, match_id->score);
	return rc;
}

/** Receive device match IDs from the device's parent driver and add them to the
 * list of devices match ids.
 *
 * @param match_count	The number of device's match ids to be received.
 * @param match_ids	The list of the device's match ids.
 * @return		Zero on success, error code otherwise.
 */
static errno_t devman_receive_match_ids(sysarg_t match_count,
    match_id_list_t *match_ids)
{
	errno_t ret = EOK;
	size_t i;

	for (i = 0; i < match_count; i++) {
		if (EOK != (ret = devman_receive_match_id(match_ids)))
			return ret;
	}
	return ret;
}

/** Handle function registration.
 *
 * Child devices are registered by their parent's device driver.
 */
static void devman_add_function(cap_call_handle_t chandle, ipc_call_t *call)
{
	fun_type_t ftype = (fun_type_t) IPC_GET_ARG1(*call);
	devman_handle_t dev_handle = IPC_GET_ARG2(*call);
	sysarg_t match_count = IPC_GET_ARG3(*call);
	dev_tree_t *tree = &device_tree;

	dev_node_t *pdev = find_dev_node(&device_tree, dev_handle);
	if (pdev == NULL) {
		async_answer_0(chandle, ENOENT);
		return;
	}

	if (ftype != fun_inner && ftype != fun_exposed) {
		/* Unknown function type */
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Unknown function type %d provided by driver.",
		    (int) ftype);

		dev_del_ref(pdev);
		async_answer_0(chandle, EINVAL);
		return;
	}

	char *fun_name = NULL;
	errno_t rc = async_data_write_accept((void **)&fun_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		dev_del_ref(pdev);
		async_answer_0(chandle, rc);
		return;
	}

	fibril_rwlock_write_lock(&tree->rwlock);

	/* Check device state */
	if (pdev->state == DEVICE_REMOVED) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		dev_del_ref(pdev);
		async_answer_0(chandle, ENOENT);
		return;
	}

	/* Check that function with same name is not there already. */
	fun_node_t *tfun = find_fun_node_in_device(tree, pdev, fun_name);
	if (tfun) {
		fun_del_ref(tfun);	/* drop the new unwanted reference */
		fibril_rwlock_write_unlock(&tree->rwlock);
		dev_del_ref(pdev);
		async_answer_0(chandle, EEXIST);
		printf(NAME ": Warning, driver tried to register `%s' twice.\n",
		    fun_name);
		free(fun_name);
		return;
	}

	fun_node_t *fun = create_fun_node();
	/* One reference for creation, one for us */
	fun_add_ref(fun);
	fun_add_ref(fun);
	fun->ftype = ftype;

	/*
	 * We can lock the function here even when holding the tree because
	 * we know it cannot be held by anyone else yet.
	 */
	fun_busy_lock(fun);

	if (!insert_fun_node(&device_tree, fun, fun_name, pdev)) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		dev_del_ref(pdev);
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		delete_fun_node(fun);
		async_answer_0(chandle, ENOMEM);
		return;
	}

	fibril_rwlock_write_unlock(&tree->rwlock);
	dev_del_ref(pdev);

	devman_receive_match_ids(match_count, &fun->match_ids);

	rc = fun_online(fun);
	if (rc != EOK) {
		/* XXX Set some failed state? */
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		async_answer_0(chandle, rc);
		return;
	}

	fun_busy_unlock(fun);
	fun_del_ref(fun);

	/* Return device handle to parent's driver. */
	async_answer_1(chandle, EOK, fun->handle);
}

static void devman_add_function_to_cat(cap_call_handle_t chandle, ipc_call_t *call)
{
	devman_handle_t handle = IPC_GET_ARG1(*call);
	category_id_t cat_id;
	errno_t rc;

	/* Get category name. */
	char *cat_name;
	rc = async_data_write_accept((void **) &cat_name, true,
	    0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		return;
	}

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(chandle, ENOENT);
		return;
	}

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(chandle, ENOENT);
		return;
	}

	rc = loc_category_get_id(cat_name, &cat_id, IPC_FLAG_BLOCKING);
	if (rc == EOK) {
		loc_service_add_to_cat(fun->service_id, cat_id);
		log_msg(LOG_DEFAULT, LVL_NOTE, "Function `%s' added to category `%s'.",
		    fun->pathname, cat_name);
	} else {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed adding function `%s' to category "
		    "`%s'.", fun->pathname, cat_name);
	}

	fibril_rwlock_read_unlock(&device_tree.rwlock);
	fun_del_ref(fun);

	async_answer_0(chandle, rc);
}

/** Online function by driver request.
 *
 */
static void devman_drv_fun_online(cap_call_handle_t icall_handle, ipc_call_t *icall,
    driver_t *drv)
{
	fun_node_t *fun;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_drv_fun_online()");

	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		async_answer_0(icall_handle, ENOENT);
		return;
	}

	fun_busy_lock(fun);

	fibril_rwlock_read_lock(&device_tree.rwlock);
	if (fun->dev == NULL || fun->dev->drv != drv) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		async_answer_0(icall_handle, ENOENT);
		return;
	}
	fibril_rwlock_read_unlock(&device_tree.rwlock);

	rc = fun_online(fun);
	if (rc != EOK) {
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		async_answer_0(icall_handle, rc);
		return;
	}

	fun_busy_unlock(fun);
	fun_del_ref(fun);

	async_answer_0(icall_handle, EOK);
}


/** Offline function by driver request.
 *
 */
static void devman_drv_fun_offline(cap_call_handle_t icall_handle, ipc_call_t *icall,
    driver_t *drv)
{
	fun_node_t *fun;
	errno_t rc;

	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		async_answer_0(icall_handle, ENOENT);
		return;
	}

	fun_busy_lock(fun);

	fibril_rwlock_write_lock(&device_tree.rwlock);
	if (fun->dev == NULL || fun->dev->drv != drv) {
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		async_answer_0(icall_handle, ENOENT);
		return;
	}
	fibril_rwlock_write_unlock(&device_tree.rwlock);

	rc = fun_offline(fun);
	if (rc != EOK) {
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		async_answer_0(icall_handle, rc);
		return;
	}

	fun_busy_unlock(fun);
	fun_del_ref(fun);
	async_answer_0(icall_handle, EOK);
}

/** Remove function. */
static void devman_remove_function(cap_call_handle_t chandle, ipc_call_t *call)
{
	devman_handle_t fun_handle = IPC_GET_ARG1(*call);
	dev_tree_t *tree = &device_tree;
	errno_t rc;

	fun_node_t *fun = find_fun_node(&device_tree, fun_handle);
	if (fun == NULL) {
		async_answer_0(chandle, ENOENT);
		return;
	}

	fun_busy_lock(fun);

	fibril_rwlock_write_lock(&tree->rwlock);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_remove_function(fun='%s')", fun->pathname);

	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		fun_busy_unlock(fun);
		fun_del_ref(fun);
		async_answer_0(chandle, ENOENT);
		return;
	}

	if (fun->ftype == fun_inner) {
		/* This is a surprise removal. Handle possible descendants */
		if (fun->child != NULL) {
			dev_node_t *dev = fun->child;
			device_state_t dev_state;
			errno_t gone_rc;

			dev_add_ref(dev);
			dev_state = dev->state;

			fibril_rwlock_write_unlock(&device_tree.rwlock);

			/* If device is owned by driver, inform driver it is gone. */
			if (dev_state == DEVICE_USABLE)
				gone_rc = driver_dev_gone(&device_tree, dev);
			else
				gone_rc = EOK;

			fibril_rwlock_read_lock(&device_tree.rwlock);

			/* Verify that driver succeeded and removed all functions */
			if (gone_rc != EOK || !list_empty(&dev->functions)) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Driver did not remove "
				    "functions for device that is gone. "
				    "Device node is now defunct.");

				/*
				 * Not much we can do but mark the device
				 * node as having invalid state. This
				 * is a driver bug.
				 */
				dev->state = DEVICE_INVALID;
				fibril_rwlock_read_unlock(&device_tree.rwlock);
				dev_del_ref(dev);
				if (gone_rc == EOK)
					gone_rc = ENOTSUP;
				fun_busy_unlock(fun);
				fun_del_ref(fun);
				async_answer_0(chandle, gone_rc);
				return;
			}

			driver_t *driver = dev->drv;
			fibril_rwlock_read_unlock(&device_tree.rwlock);

			if (driver)
				detach_driver(&device_tree, dev);

			fibril_rwlock_write_lock(&device_tree.rwlock);
			remove_dev_node(&device_tree, dev);

			/* Delete ref created when node was inserted */
			dev_del_ref(dev);
			/* Delete ref created by dev_add_ref(dev) above */
			dev_del_ref(dev);
		}
	} else {
		if (fun->service_id != 0) {
			/* Unregister from location service */
			rc = loc_unregister_tree_function(fun, &device_tree);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Failed unregistering tree "
				    "service.");
				fibril_rwlock_write_unlock(&tree->rwlock);
				fun_busy_unlock(fun);
				fun_del_ref(fun);
				async_answer_0(chandle, EIO);
				return;
			}
		}
	}

	remove_fun_node(&device_tree, fun);
	fibril_rwlock_write_unlock(&tree->rwlock);
	fun_busy_unlock(fun);

	/* Delete ref added when inserting function into tree */
	fun_del_ref(fun);
	/* Delete ref added above when looking up function */
	fun_del_ref(fun);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_remove_function() succeeded.");
	async_answer_0(chandle, EOK);
}

/** Initialize driver which has registered itself as running and ready.
 *
 * The initialization is done in a separate fibril to avoid deadlocks (if the
 * driver needed to be served by devman during the driver's initialization).
 */
static errno_t init_running_drv(void *drv)
{
	driver_t *driver = (driver_t *) drv;

	initialize_running_driver(driver, &device_tree);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "The `%s` driver was successfully initialized.",
	    driver->name);
	return 0;
}

/** Function for handling connections from a driver to the device manager. */
void devman_connection_driver(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	client_t *client;
	driver_t *driver = NULL;

	/* Accept the connection. */
	async_answer_0(icall_handle, EOK);

	client = async_get_client_data();
	if (client == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to allocate client data.");
		return;
	}

	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call))
			break;

		if (IPC_GET_IMETHOD(call) != DEVMAN_DRIVER_REGISTER) {
			fibril_mutex_lock(&client->mutex);
			driver = client->driver;
			fibril_mutex_unlock(&client->mutex);
			if (driver == NULL) {
				/* First call must be to DEVMAN_DRIVER_REGISTER */
				async_answer_0(chandle, ENOTSUP);
				continue;
			}
		}

		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAN_DRIVER_REGISTER:
			fibril_mutex_lock(&client->mutex);
			if (client->driver != NULL) {
				fibril_mutex_unlock(&client->mutex);
				async_answer_0(chandle, EINVAL);
				continue;
			}
			client->driver = devman_driver_register(chandle, &call);
			fibril_mutex_unlock(&client->mutex);
			break;
		case DEVMAN_ADD_FUNCTION:
			devman_add_function(chandle, &call);
			break;
		case DEVMAN_ADD_DEVICE_TO_CATEGORY:
			devman_add_function_to_cat(chandle, &call);
			break;
		case DEVMAN_DRV_FUN_ONLINE:
			devman_drv_fun_online(chandle, &call, driver);
			break;
		case DEVMAN_DRV_FUN_OFFLINE:
			devman_drv_fun_offline(chandle, &call, driver);
			break;
		case DEVMAN_REMOVE_FUNCTION:
			devman_remove_function(chandle, &call);
			break;
		default:
			async_answer_0(chandle, EINVAL);
			break;
		}
	}
}

/** @}
 */
