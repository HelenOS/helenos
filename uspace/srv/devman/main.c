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
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <ctype.h>
#include <io/log.h>
#include <ipc/devman.h>
#include <ipc/driver.h>
#include <loc.h>

#include "client_conn.h"
#include "dev.h"
#include "devman.h"
#include "devtree.h"
#include "drv_conn.h"
#include "driver.h"
#include "fun.h"
#include "loc.h"

#define DRIVER_DEFAULT_STORE  "/drv"

driver_list_t drivers_list;
dev_tree_t device_tree;

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
		log_msg(LOG_DEFAULT, LVL_ERROR, "IPC forwarding failed - no device or "
		    "function with handle %" PRIun " was found.", handle);
		async_answer_0(iid, ENOENT);
		goto cleanup;
	}

	if (fun == NULL && !drv_to_parent) {
		log_msg(LOG_DEFAULT, LVL_ERROR, NAME ": devman_forward error - cannot "
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
	} else {
		/* Connect to the specified function */
		driver = dev->drv;
		fwd_h = handle;
	}
	
	fibril_rwlock_read_unlock(&device_tree.rwlock);
	
	if (driver == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "IPC forwarding refused - " \
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
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Could not forward to driver `%s'.", driver->name);
		async_answer_0(iid, EINVAL);
		goto cleanup;
	}

	if (fun != NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "Forwarding request for `%s' function to driver `%s'.",
		    fun->pathname, driver->name);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG,
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
		log_msg(LOG_DEFAULT, LVL_WARN, "devman_connection_loc(): function "
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
	
	log_msg(LOG_DEFAULT, LVL_DEBUG,
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_init - looking for available drivers.");
	
	/* Initialize list of available drivers. */
	init_driver_list(&drivers_list);
	if (lookup_available_drivers(&drivers_list,
	    DRIVER_DEFAULT_STORE) == 0) {
		log_msg(LOG_DEFAULT, LVL_FATAL, "No drivers found.");
		return false;
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "devman_init - list of drivers has been initialized.");
	
	/* Create root device node. */
	if (!init_device_tree(&device_tree, &drivers_list)) {
		log_msg(LOG_DEFAULT, LVL_FATAL, "Failed to initialize device tree.");
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
	
	int rc = log_init(NAME);
	if (rc != EOK) {
		printf("%s: Error initializing logging subsystem.\n", NAME);
		return rc;
	}
	
	/* Set handlers for incoming connections. */
	async_set_client_data_constructor(devman_client_data_create);
	async_set_client_data_destructor(devman_client_data_destroy);
	async_set_client_connection(devman_connection);
	
	if (!devman_init()) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error while initializing service.");
		return -1;
	}
	
	/* Register device manager at naming service. */
	rc = service_register(SERVICE_DEVMAN);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering as a service.");
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
