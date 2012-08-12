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

#include <inttypes.h>
#include <assert.h>
#include <ipc/services.h>
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <io/log.h>
#include <ipc/devman.h>
#include <ipc/driver.h>
#include <thread.h>
#include <loc.h>

#include "devman.h"

#define DRIVER_DEFAULT_STORE  "/drv"

static driver_list_t drivers_list;
static dev_tree_t device_tree;

static int init_running_drv(void *drv);

/** Register running driver. */
static driver_t *devman_driver_register(ipc_callid_t callid, ipc_call_t *call)
{
	driver_t *driver = NULL;
	char *drv_name = NULL;

	log_msg(LVL_DEBUG, "devman_driver_register");
	
	/* Get driver name. */
	int rc = async_data_write_accept((void **) &drv_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return NULL;
	}

	log_msg(LVL_DEBUG, "The `%s' driver is trying to register.",
	    drv_name);
	
	/* Find driver structure. */
	driver = find_driver(&drivers_list, drv_name);
	if (driver == NULL) {
		log_msg(LVL_ERROR, "No driver named `%s' was found.", drv_name);
		free(drv_name);
		drv_name = NULL;
		async_answer_0(callid, ENOENT);
		return NULL;
	}
	
	free(drv_name);
	drv_name = NULL;
	
	fibril_mutex_lock(&driver->driver_mutex);
	
	if (driver->sess) {
		/* We already have a connection to the driver. */
		log_msg(LVL_ERROR, "Driver '%s' already started.\n",
		    driver->name);
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(callid, EEXISTS);
		return NULL;
	}
	
	switch (driver->state) {
	case DRIVER_NOT_STARTED:
		/* Somebody started the driver manually. */
		log_msg(LVL_NOTE, "Driver '%s' started manually.\n",
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
	log_msg(LVL_DEBUG, "Creating connection to the `%s' driver.",
	    driver->name);
	driver->sess = async_callback_receive(EXCHANGE_PARALLEL);
	if (!driver->sess) {
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(callid, ENOTSUP);
		return NULL;
	}
	/* FIXME: Work around problem with callback sessions */
	async_sess_args_set(driver->sess, DRIVER_DEVMAN, 0, 0);
	
	log_msg(LVL_NOTE,
	    "The `%s' driver was successfully registered as running.",
	    driver->name);
	
	/*
	 * Initialize the driver as running (e.g. pass assigned devices to it)
	 * in a separate fibril; the separate fibril is used to enable the
	 * driver to use devman service during the driver's initialization.
	 */
	fid_t fid = fibril_create(init_running_drv, driver);
	if (fid == 0) {
		log_msg(LVL_ERROR, "Failed to create initialization fibril " \
		    "for driver `%s'.", driver->name);
		fibril_mutex_unlock(&driver->driver_mutex);
		async_answer_0(callid, ENOMEM);
		return NULL;
	}
	
	fibril_add_ready(fid);
	fibril_mutex_unlock(&driver->driver_mutex);
	
	async_answer_0(callid, EOK);
	return driver;
}

/** Receive device match ID from the device's parent driver and add it to the
 * list of devices match ids.
 *
 * @param match_ids	The list of the device's match ids.
 * @return		Zero on success, negative error code otherwise.
 */
static int devman_receive_match_id(match_id_list_t *match_ids)
{
	match_id_t *match_id = create_match_id();
	ipc_callid_t callid;
	ipc_call_t call;
	int rc = 0;
	
	callid = async_get_call(&call);
	if (DEVMAN_ADD_MATCH_ID != IPC_GET_IMETHOD(call)) {
		log_msg(LVL_ERROR, 
		    "Invalid protocol when trying to receive match id.");
		async_answer_0(callid, EINVAL); 
		delete_match_id(match_id);
		return EINVAL;
	}
	
	if (match_id == NULL) {
		log_msg(LVL_ERROR, "Failed to allocate match id.");
		async_answer_0(callid, ENOMEM);
		return ENOMEM;
	}
	
	async_answer_0(callid, EOK);
	
	match_id->score = IPC_GET_ARG1(call);
	
	char *match_id_str;
	rc = async_data_write_accept((void **) &match_id_str, true, 0, 0, 0, 0);
	match_id->id = match_id_str;
	if (rc != EOK) {
		delete_match_id(match_id);
		log_msg(LVL_ERROR, "Failed to receive match id string: %s.",
		    str_error(rc));
		return rc;
	}
	
	list_append(&match_id->link, &match_ids->ids);
	
	log_msg(LVL_DEBUG, "Received match id `%s', score %d.",
	    match_id->id, match_id->score);
	return rc;
}

/** Receive device match IDs from the device's parent driver and add them to the
 * list of devices match ids.
 *
 * @param match_count	The number of device's match ids to be received.
 * @param match_ids	The list of the device's match ids.
 * @return		Zero on success, negative error code otherwise.
 */
static int devman_receive_match_ids(sysarg_t match_count,
    match_id_list_t *match_ids)
{
	int ret = EOK;
	size_t i;
	
	for (i = 0; i < match_count; i++) {
		if (EOK != (ret = devman_receive_match_id(match_ids)))
			return ret;
	}
	return ret;
}

static int assign_driver_fibril(void *arg)
{
	dev_node_t *dev_node = (dev_node_t *) arg;
	assign_driver(dev_node, &drivers_list, &device_tree);

	/* Delete one reference we got from the caller. */
	dev_del_ref(dev_node);
	return EOK;
}

static int online_function(fun_node_t *fun)
{
	dev_node_t *dev;
	
	fibril_rwlock_write_lock(&device_tree.rwlock);
	
	if (fun->state == FUN_ON_LINE) {
		fibril_rwlock_write_unlock(&device_tree.rwlock);
		log_msg(LVL_WARN, "Function %s is already on line.",
		    fun->pathname);
		return EOK;
	}
	
	if (fun->ftype == fun_inner) {
		dev = create_dev_node();
		if (dev == NULL) {
			fibril_rwlock_write_unlock(&device_tree.rwlock);
			return ENOMEM;
		}
		
		insert_dev_node(&device_tree, dev, fun);
		dev_add_ref(dev);
	}
	
	log_msg(LVL_DEBUG, "devman_add_function(fun=\"%s\")", fun->pathname);
	
	if (fun->ftype == fun_inner) {
		dev = fun->child;
		assert(dev != NULL);
		
		/* Give one reference over to assign_driver_fibril(). */
		dev_add_ref(dev);
		
		/*
		 * Try to find a suitable driver and assign it to the device.  We do
		 * not want to block the current fibril that is used for processing
		 * incoming calls: we will launch a separate fibril to handle the
		 * driver assigning. That is because assign_driver can actually include
		 * task spawning which could take some time.
		 */
		fid_t assign_fibril = fibril_create(assign_driver_fibril, dev);
		if (assign_fibril == 0) {
			log_msg(LVL_ERROR, "Failed to create fibril for "
			    "assigning driver.");
			/* XXX Cleanup */
			fibril_rwlock_write_unlock(&device_tree.rwlock);
			return ENOMEM;
		}
		fibril_add_ready(assign_fibril);
	} else
		loc_register_tree_function(fun, &device_tree);
	
	fibril_rwlock_write_unlock(&device_tree.rwlock);
	
	return EOK;
}

static int offline_function(fun_node_t *fun)
{
	int rc;
	
	fibril_rwlock_write_lock(&device_tree.rwlock);
	
	if (fun->state == FUN_OFF_LINE) {
		fibril_rwlock_write_unlock(&device_tree.rwlock);
		log_msg(LVL_WARN, "Function %s is already off line.",
		    fun->pathname);
		return EOK;
	}
	
	if (fun->ftype == fun_inner) {
		log_msg(LVL_DEBUG, "Offlining inner function %s.",
		    fun->pathname);
		
		if (fun->child != NULL) {
			dev_node_t *dev = fun->child;
			device_state_t dev_state;
			
			dev_add_ref(dev);
			dev_state = dev->state;
			
			fibril_rwlock_write_unlock(&device_tree.rwlock);

			/* If device is owned by driver, ask driver to give it up. */
			if (dev_state == DEVICE_USABLE) {
				rc = driver_dev_remove(&device_tree, dev);
				if (rc != EOK) {
					dev_del_ref(dev);
					return ENOTSUP;
				}
			}
			
			/* Verify that driver removed all functions */
			fibril_rwlock_read_lock(&device_tree.rwlock);
			if (!list_empty(&dev->functions)) {
				fibril_rwlock_read_unlock(&device_tree.rwlock);
				dev_del_ref(dev);
				return EIO;
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
		/* Unregister from location service */
		rc = loc_service_unregister(fun->service_id);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&device_tree.rwlock);
			log_msg(LVL_ERROR, "Failed unregistering tree service.");
			return EIO;
		}
		
		fun->service_id = 0;
	}
	
	fun->state = FUN_OFF_LINE;
	fibril_rwlock_write_unlock(&device_tree.rwlock);
	
	return EOK;
}

/** Handle function registration.
 *
 * Child devices are registered by their parent's device driver.
 */
static void devman_add_function(ipc_callid_t callid, ipc_call_t *call)
{
	fun_type_t ftype = (fun_type_t) IPC_GET_ARG1(*call);
	devman_handle_t dev_handle = IPC_GET_ARG2(*call);
	sysarg_t match_count = IPC_GET_ARG3(*call);
	dev_tree_t *tree = &device_tree;
	
	dev_node_t *pdev = find_dev_node(&device_tree, dev_handle);
	if (pdev == NULL) {
		async_answer_0(callid, ENOENT);
		return;
	}
	
	if (ftype != fun_inner && ftype != fun_exposed) {
		/* Unknown function type */
		log_msg(LVL_ERROR, 
		    "Unknown function type %d provided by driver.",
		    (int) ftype);

		dev_del_ref(pdev);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	char *fun_name = NULL;
	int rc = async_data_write_accept((void **)&fun_name, true, 0, 0, 0, 0);
	if (rc != EOK) {
		dev_del_ref(pdev);
		async_answer_0(callid, rc);
		return;
	}
	
	fibril_rwlock_write_lock(&tree->rwlock);
	
	/* Check device state */
	if (pdev->state == DEVICE_REMOVED) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		dev_del_ref(pdev);
		async_answer_0(callid, ENOENT);
		return;
	}
	
	/* Check that function with same name is not there already. */
	fun_node_t *tfun = find_fun_node_in_device(tree, pdev, fun_name);
	if (tfun) {
		fun_del_ref(tfun);	/* drop the new unwanted reference */
		fibril_rwlock_write_unlock(&tree->rwlock);
		dev_del_ref(pdev);
		async_answer_0(callid, EEXISTS);
		printf(NAME ": Warning, driver tried to register `%s' twice.\n",
		    fun_name);
		free(fun_name);
		return;
	}
	
	fun_node_t *fun = create_fun_node();
	fun_add_ref(fun);
	fun->ftype = ftype;
	
	if (!insert_fun_node(&device_tree, fun, fun_name, pdev)) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		dev_del_ref(pdev);
		delete_fun_node(fun);
		async_answer_0(callid, ENOMEM);
		return;
	}
	
	fibril_rwlock_write_unlock(&tree->rwlock);
	dev_del_ref(pdev);
	
	devman_receive_match_ids(match_count, &fun->match_ids);
	
	rc = online_function(fun);
	if (rc != EOK) {
		/* XXX clean up */
		async_answer_0(callid, rc);
		return;
	}
	
	/* Return device handle to parent's driver. */
	async_answer_1(callid, EOK, fun->handle);
}

static void devman_add_function_to_cat(ipc_callid_t callid, ipc_call_t *call)
{
	devman_handle_t handle = IPC_GET_ARG1(*call);
	category_id_t cat_id;
	int rc;
	
	/* Get category name. */
	char *cat_name;
	rc = async_data_write_accept((void **) &cat_name, true,
	    0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}
	
	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(callid, ENOENT);
		return;
	}
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, ENOENT);
		return;
	}
	
	rc = loc_category_get_id(cat_name, &cat_id, IPC_FLAG_BLOCKING);
	if (rc == EOK) {
		loc_service_add_to_cat(fun->service_id, cat_id);
		log_msg(LVL_NOTE, "Function `%s' added to category `%s'.",
		    fun->pathname, cat_name);
	} else {
		log_msg(LVL_ERROR, "Failed adding function `%s' to category "
		    "`%s'.", fun->pathname, cat_name);
	}
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	fun_del_ref(fun);
	
	async_answer_0(callid, rc);
}

/** Online function by driver request.
 *
 */
static void devman_drv_fun_online(ipc_callid_t iid, ipc_call_t *icall,
    driver_t *drv)
{
	fun_node_t *fun;
	int rc;
	
	log_msg(LVL_DEBUG, "devman_drv_fun_online()");
	
	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	if (fun->dev == NULL || fun->dev->drv != drv) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		fun_del_ref(fun);
		async_answer_0(iid, ENOENT);
		return;
	}
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	
	rc = online_function(fun);
	if (rc != EOK) {
		fun_del_ref(fun);
		async_answer_0(iid, (sysarg_t) rc);
		return;
	}
	
	fun_del_ref(fun);
	
	async_answer_0(iid, (sysarg_t) EOK);
}


/** Offline function by driver request.
 *
 */
static void devman_drv_fun_offline(ipc_callid_t iid, ipc_call_t *icall,
    driver_t *drv)
{
	fun_node_t *fun;
	int rc;

	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	fibril_rwlock_write_lock(&device_tree.rwlock);
	if (fun->dev == NULL || fun->dev->drv != drv) {
		fun_del_ref(fun);
		async_answer_0(iid, ENOENT);
		return;
	}
	fibril_rwlock_write_unlock(&device_tree.rwlock);
	
	rc = offline_function(fun);
	if (rc != EOK) {
		fun_del_ref(fun);
		async_answer_0(iid, (sysarg_t) rc);
		return;
	}
	
	fun_del_ref(fun);
	async_answer_0(iid, (sysarg_t) EOK);
}

/** Remove function. */
static void devman_remove_function(ipc_callid_t callid, ipc_call_t *call)
{
	devman_handle_t fun_handle = IPC_GET_ARG1(*call);
	dev_tree_t *tree = &device_tree;
	int rc;
	
	fun_node_t *fun = find_fun_node(&device_tree, fun_handle);
	if (fun == NULL) {
		async_answer_0(callid, ENOENT);
		return;
	}
	
	fibril_rwlock_write_lock(&tree->rwlock);
	
	log_msg(LVL_DEBUG, "devman_remove_function(fun='%s')", fun->pathname);
	
	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_write_unlock(&tree->rwlock);
		async_answer_0(callid, ENOENT);
		return;
	}
	
	if (fun->ftype == fun_inner) {
		/* This is a surprise removal. Handle possible descendants */
		if (fun->child != NULL) {
			dev_node_t *dev = fun->child;
			device_state_t dev_state;
			int gone_rc;
			
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
				log_msg(LVL_ERROR, "Driver did not remove "
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
				async_answer_0(callid, gone_rc);
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
			rc = loc_service_unregister(fun->service_id);
			if (rc != EOK) {
				log_msg(LVL_ERROR, "Failed unregistering tree "
				    "service.");
				fibril_rwlock_write_unlock(&tree->rwlock);
				fun_del_ref(fun);
				async_answer_0(callid, EIO);
				return;
			}
		}
	}
	
	remove_fun_node(&device_tree, fun);
	fibril_rwlock_write_unlock(&tree->rwlock);
	
	/* Delete ref added when inserting function into tree */
	fun_del_ref(fun);
	/* Delete ref added above when looking up function */
	fun_del_ref(fun);
	
	log_msg(LVL_DEBUG, "devman_remove_function() succeeded.");
	async_answer_0(callid, EOK);
}

/** Initialize driver which has registered itself as running and ready.
 *
 * The initialization is done in a separate fibril to avoid deadlocks (if the
 * driver needed to be served by devman during the driver's initialization).
 */
static int init_running_drv(void *drv)
{
	driver_t *driver = (driver_t *) drv;
	
	initialize_running_driver(driver, &device_tree);
	log_msg(LVL_DEBUG, "The `%s` driver was successfully initialized.",
	    driver->name);
	return 0;
}

/** Function for handling connections from a driver to the device manager. */
static void devman_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	client_t *client;
	driver_t *driver;
	
	/* Accept the connection. */
	async_answer_0(iid, EOK);
	
	client = async_get_client_data();
	if (client == NULL) {
		log_msg(LVL_ERROR, "Failed to allocate client data.");
		return;
	}
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		if (IPC_GET_IMETHOD(call) != DEVMAN_DRIVER_REGISTER) {
			fibril_mutex_lock(&client->mutex);
			driver = client->driver;
			fibril_mutex_unlock(&client->mutex);
			if (driver == NULL) {
				/* First call must be to DEVMAN_DRIVER_REGISTER */
				async_answer_0(callid, ENOTSUP);
				continue;
			}
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAN_DRIVER_REGISTER:
			fibril_mutex_lock(&client->mutex);
			if (client->driver != NULL) {
				fibril_mutex_unlock(&client->mutex);
				async_answer_0(callid, EINVAL);
				continue;
			}
			client->driver = devman_driver_register(callid, &call);
			fibril_mutex_unlock(&client->mutex);
			break;
		case DEVMAN_ADD_FUNCTION:
			devman_add_function(callid, &call);
			break;
		case DEVMAN_ADD_DEVICE_TO_CATEGORY:
			devman_add_function_to_cat(callid, &call);
			break;
		case DEVMAN_DRV_FUN_ONLINE:
			devman_drv_fun_online(callid, &call, driver);
			break;
		case DEVMAN_DRV_FUN_OFFLINE:
			devman_drv_fun_offline(callid, &call, driver);
			break;
		case DEVMAN_REMOVE_FUNCTION:
			devman_remove_function(callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Find handle for the device instance identified by the device's path in the
 * device tree. */
static void devman_function_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	char *pathname;
	devman_handle_t handle;
	
	int rc = async_data_write_accept((void **) &pathname, true, 0, 0, 0, 0);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	fun_node_t *fun = find_fun_node_by_path(&device_tree, pathname);
	
	free(pathname);

	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}
	handle = fun->handle;

	fibril_rwlock_read_unlock(&device_tree.rwlock);

	/* Delete reference created above by find_fun_node_by_path() */
	fun_del_ref(fun);

	async_answer_1(iid, EOK, handle);
}

/** Get device name. */
static void devman_fun_get_name(ipc_callid_t iid, ipc_call_t *icall)
{
	devman_handle_t handle = IPC_GET_ARG1(*icall);

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	ipc_callid_t data_callid;
	size_t data_len;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EINVAL);
		fun_del_ref(fun);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		fun_del_ref(fun);
		return;
	}

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		free(buffer);

		async_answer_0(data_callid, ENOENT);
		async_answer_0(iid, ENOENT);
		fun_del_ref(fun);
		return;
	}

	size_t sent_length = str_size(fun->name);
	if (sent_length > data_len) {
		sent_length = data_len;
	}

	async_data_read_finalize(data_callid, fun->name, sent_length);
	async_answer_0(iid, EOK);

	fibril_rwlock_read_unlock(&device_tree.rwlock);
	fun_del_ref(fun);
	free(buffer);
}

/** Get function driver name. */
static void devman_fun_get_driver_name(ipc_callid_t iid, ipc_call_t *icall)
{
	devman_handle_t handle = IPC_GET_ARG1(*icall);

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	ipc_callid_t data_callid;
	size_t data_len;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EINVAL);
		fun_del_ref(fun);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		fun_del_ref(fun);
		return;
	}

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		free(buffer);

		async_answer_0(data_callid, ENOENT);
		async_answer_0(iid, ENOENT);
		fun_del_ref(fun);
		return;
	}

	/* Check whether function has a driver */
	if (fun->child == NULL || fun->child->drv == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		free(buffer);

		async_answer_0(data_callid, EINVAL);
		async_answer_0(iid, EINVAL);
		fun_del_ref(fun);
		return;
	}

	size_t sent_length = str_size(fun->child->drv->name);
	if (sent_length > data_len) {
		sent_length = data_len;
	}

	async_data_read_finalize(data_callid, fun->child->drv->name,
	    sent_length);
	async_answer_0(iid, EOK);

	fibril_rwlock_read_unlock(&device_tree.rwlock);
	fun_del_ref(fun);
	free(buffer);
}

/** Get device path. */
static void devman_fun_get_path(ipc_callid_t iid, ipc_call_t *icall)
{
	devman_handle_t handle = IPC_GET_ARG1(*icall);

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	ipc_callid_t data_callid;
	size_t data_len;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EINVAL);
		fun_del_ref(fun);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		fun_del_ref(fun);
		return;
	}
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		free(buffer);

		async_answer_0(data_callid, ENOENT);
		async_answer_0(iid, ENOENT);
		fun_del_ref(fun);
		return;
	}
	
	size_t sent_length = str_size(fun->pathname);
	if (sent_length > data_len) {
		sent_length = data_len;
	}

	async_data_read_finalize(data_callid, fun->pathname, sent_length);
	async_answer_0(iid, EOK);

	fibril_rwlock_read_unlock(&device_tree.rwlock);
	fun_del_ref(fun);
	free(buffer);
}

static void devman_dev_get_functions(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	size_t act_size;
	int rc;
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	dev_node_t *dev = find_dev_node_no_lock(&device_tree,
	    IPC_GET_ARG1(*icall));
	if (dev == NULL || dev->state == DEVICE_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, ENOENT);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	devman_handle_t *hdl_buf = (devman_handle_t *) malloc(size);
	if (hdl_buf == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	rc = dev_get_functions(&device_tree, dev, hdl_buf, size, &act_size);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	
	sysarg_t retval = async_data_read_finalize(callid, hdl_buf, size);
	free(hdl_buf);
	
	async_answer_1(iid, retval, act_size);
}


/** Get handle for child device of a function. */
static void devman_fun_get_child(ipc_callid_t iid, ipc_call_t *icall)
{
	fun_node_t *fun;
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	fun = find_fun_node_no_lock(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL || fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	if (fun->child == NULL) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	async_answer_1(iid, EOK, fun->child->handle);
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
}

/** Online function.
 *
 * Send a request to online a function to the responsible driver.
 * The driver may offline other functions if necessary (i.e. if the state
 * of this function is linked to state of another function somehow).
 */
static void devman_fun_online(ipc_callid_t iid, ipc_call_t *icall)
{
	fun_node_t *fun;
	int rc;

	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	rc = driver_fun_online(&device_tree, fun);
	fun_del_ref(fun);
	
	async_answer_0(iid, (sysarg_t) rc);
}

/** Offline function.
 *
 * Send a request to offline a function to the responsible driver. As
 * a result the subtree rooted at that function should be cleanly
 * detatched. The driver may offline other functions if necessary
 * (i.e. if the state of this function is linked to state of another
 * function somehow).
 */
static void devman_fun_offline(ipc_callid_t iid, ipc_call_t *icall)
{
	fun_node_t *fun;
	int rc;

	fun = find_fun_node(&device_tree, IPC_GET_ARG1(*icall));
	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	rc = driver_fun_offline(&device_tree, fun);
	fun_del_ref(fun);
	
	async_answer_0(iid, (sysarg_t) rc);
}

/** Find handle for the function instance identified by its service ID. */
static void devman_fun_sid_to_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	fun_node_t *fun;

	fun = find_loc_tree_function(&device_tree, IPC_GET_ARG1(*icall));
	
	if (fun == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Check function state */
	if (fun->state == FUN_REMOVED) {
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}

	async_answer_1(iid, EOK, fun->handle);
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	fun_del_ref(fun);
}

/** Function for handling connections from a client to the device manager. */
static void devman_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection. */
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case DEVMAN_DEVICE_GET_HANDLE:
			devman_function_get_handle(callid, &call);
			break;
		case DEVMAN_DEV_GET_FUNCTIONS:
			devman_dev_get_functions(callid, &call);
			break;
		case DEVMAN_FUN_GET_CHILD:
			devman_fun_get_child(callid, &call);
			break;
		case DEVMAN_FUN_GET_NAME:
			devman_fun_get_name(callid, &call);
			break;
		case DEVMAN_FUN_GET_DRIVER_NAME:
			devman_fun_get_driver_name(callid, &call);
			break;
		case DEVMAN_FUN_GET_PATH:
			devman_fun_get_path(callid, &call);
			break;
		case DEVMAN_FUN_ONLINE:
			devman_fun_online(callid, &call);
			break;
		case DEVMAN_FUN_OFFLINE:
			devman_fun_offline(callid, &call);
			break;
		case DEVMAN_FUN_SID_TO_HANDLE:
			devman_fun_sid_to_handle(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOENT);
		}
	}
}

static void devman_forward(ipc_callid_t iid, ipc_call_t *icall,
    bool drv_to_parent)
{
	devman_handle_t handle = IPC_GET_ARG2(*icall);
	devman_handle_t fwd_h;
	fun_node_t *fun = NULL;
	dev_node_t *dev = NULL;
	
	fun = find_fun_node(&device_tree, handle);
	if (fun == NULL)
		dev = find_dev_node(&device_tree, handle);
	else {
		fibril_rwlock_read_lock(&device_tree.rwlock);
		dev = fun->dev;
		if (dev != NULL)
			dev_add_ref(dev);
		fibril_rwlock_read_unlock(&device_tree.rwlock);
	}

	/*
	 * For a valid function to connect to we need a device. The root
	 * function, for example, has no device and cannot be connected to.
	 * This means @c dev needs to be valid regardless whether we are
	 * connecting to a device or to a function.
	 */
	if (dev == NULL) {
		log_msg(LVL_ERROR, "IPC forwarding failed - no device or "
		    "function with handle %" PRIun " was found.", handle);
		async_answer_0(iid, ENOENT);
		goto cleanup;
	}

	if (fun == NULL && !drv_to_parent) {
		log_msg(LVL_ERROR, NAME ": devman_forward error - cannot "
		    "connect to handle %" PRIun ", refers to a device.",
		    handle);
		async_answer_0(iid, ENOENT);
		goto cleanup;
	}
	
	driver_t *driver = NULL;
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	if (drv_to_parent) {
		/* Connect to parent function of a device (or device function). */
		if (dev->pfun->dev != NULL)
			driver = dev->pfun->dev->drv;
		
		fwd_h = dev->pfun->handle;
	} else if (dev->state == DEVICE_USABLE) {
		/* Connect to the specified function */
		driver = dev->drv;
		assert(driver != NULL);
		
		fwd_h = handle;
	}
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	
	if (driver == NULL) {
		log_msg(LVL_ERROR, "IPC forwarding refused - " \
		    "the device %" PRIun " is not in usable state.", handle);
		async_answer_0(iid, ENOENT);
		goto cleanup;
	}
	
	int method;
	if (drv_to_parent)
		method = DRIVER_DRIVER;
	else
		method = DRIVER_CLIENT;
	
	if (!driver->sess) {
		log_msg(LVL_ERROR,
		    "Could not forward to driver `%s'.", driver->name);
		async_answer_0(iid, EINVAL);
		goto cleanup;
	}

	if (fun != NULL) {
		log_msg(LVL_DEBUG,
		    "Forwarding request for `%s' function to driver `%s'.",
		    fun->pathname, driver->name);
	} else {
		log_msg(LVL_DEBUG,
		    "Forwarding request for `%s' device to driver `%s'.",
		    dev->pfun->pathname, driver->name);
	}
	
	async_exch_t *exch = async_exchange_begin(driver->sess);
	async_forward_fast(iid, exch, method, fwd_h, 0, IPC_FF_NONE);
	async_exchange_end(exch);
	
cleanup:
	if (dev != NULL)
		dev_del_ref(dev);
	
	if (fun != NULL)
		fun_del_ref(fun);
}

/** Function for handling connections from a client forwarded by the location
 * service to the device manager. */
static void devman_connection_loc(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t service_id = IPC_GET_ARG2(*icall);
	fun_node_t *fun;
	dev_node_t *dev;
	devman_handle_t handle;
	driver_t *driver;

	fun = find_loc_tree_function(&device_tree, service_id);
	
	fibril_rwlock_read_lock(&device_tree.rwlock);
	
	if (fun == NULL || fun->dev == NULL || fun->dev->drv == NULL) {
		log_msg(LVL_WARN, "devman_connection_loc(): function "
		    "not found.\n");
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	dev = fun->dev;
	driver = dev->drv;
	handle = fun->handle;
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	
	async_exch_t *exch = async_exchange_begin(driver->sess);
	async_forward_fast(iid, exch, DRIVER_CLIENT, handle, 0,
	    IPC_FF_NONE);
	async_exchange_end(exch);
	
	log_msg(LVL_DEBUG,
	    "Forwarding loc service request for `%s' function to driver `%s'.",
	    fun->pathname, driver->name);

	fun_del_ref(fun);
}

/** Function for handling connections to device manager. */
static void devman_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Select port. */
	switch ((sysarg_t) (IPC_GET_ARG1(*icall))) {
	case DEVMAN_DRIVER:
		devman_connection_driver(iid, icall);
		break;
	case DEVMAN_CLIENT:
		devman_connection_client(iid, icall);
		break;
	case DEVMAN_CONNECT_TO_DEVICE:
		/* Connect client to selected device. */
		devman_forward(iid, icall, false);
		break;
	case DEVMAN_CONNECT_FROM_LOC:
		/* Someone connected through loc node. */
		devman_connection_loc(iid, icall);
		break;
	case DEVMAN_CONNECT_TO_PARENTS_DEVICE:
		/* Connect client to selected device. */
		devman_forward(iid, icall, true);
		break;
	default:
		/* No such interface */
		async_answer_0(iid, ENOENT);
	}
}

static void *devman_client_data_create(void)
{
	client_t *client;
	
	client = calloc(1, sizeof(client_t));
	if (client == NULL)
		return NULL;
	
	fibril_mutex_initialize(&client->mutex);
	return client;
}

static void devman_client_data_destroy(void *data)
{
	free(data);
}

/** Initialize device manager internal structures. */
static bool devman_init(void)
{
	log_msg(LVL_DEBUG, "devman_init - looking for available drivers.");
	
	/* Initialize list of available drivers. */
	init_driver_list(&drivers_list);
	if (lookup_available_drivers(&drivers_list,
	    DRIVER_DEFAULT_STORE) == 0) {
		log_msg(LVL_FATAL, "No drivers found.");
		return false;
	}
	
	log_msg(LVL_DEBUG, "devman_init - list of drivers has been initialized.");
	
	/* Create root device node. */
	if (!init_device_tree(&device_tree, &drivers_list)) {
		log_msg(LVL_FATAL, "Failed to initialize device tree.");
		return false;
	}
	
	/*
	 * Caution: As the device manager is not a real loc
	 * driver (it uses a completely different IPC protocol
	 * than an ordinary loc driver), forwarding a connection
	 * from client to the devman by location service will
	 * not work.
	 */
	loc_server_register(NAME);
	
	return true;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS Device Manager\n", NAME);
	
	int rc = log_init(NAME, LVL_WARN);
	if (rc != EOK) {
		printf("%s: Error initializing logging subsystem.\n", NAME);
		return rc;
	}
	
	/* Set handlers for incoming connections. */
	async_set_client_data_constructor(devman_client_data_create);
	async_set_client_data_destructor(devman_client_data_destroy);
	async_set_client_connection(devman_connection);
	
	if (!devman_init()) {
		log_msg(LVL_ERROR, "Error while initializing service.");
		return -1;
	}
	
	/* Register device manager at naming service. */
	rc = service_register(SERVICE_DEVMAN);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering as a service.");
		return rc;
	}
	
	printf("%s: Accepting connections.\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Never reached. */
	return 0;
}

/** @}
 */
