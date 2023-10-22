/*
 * Copyright (c) 2023 Jiri Svoboda
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
 * @addtogroup devman
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
#include <ctype.h>
#include <io/log.h>
#include <ipc/devman.h>
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
loc_srv_t *devman_srv;

static void devman_connection_device(ipc_call_t *icall, void *arg)
{
	devman_handle_t handle = ipc_get_arg2(icall);
	dev_node_t *dev = NULL;

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		dev = find_dev_node(&device_tree, handle);
	} else {
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
		async_answer_0(icall, ENOENT);
		goto cleanup;
	}

	if (fun == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, NAME ": devman_forward error - cannot "
		    "connect to handle %" PRIun ", refers to a device.",
		    handle);
		async_answer_0(icall, ENOENT);
		goto cleanup;
	}

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Connect to the specified function */
	driver_t *driver = dev->drv;

	fibril_rwlock_read_unlock(&device_tree.rwlock);

	if (driver == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "IPC forwarding refused - "
		    "the device %" PRIun " is not in usable state.", handle);
		async_answer_0(icall, ENOENT);
		goto cleanup;
	}

	if (!driver->sess) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Could not forward to driver `%s'.", driver->name);
		async_answer_0(icall, EINVAL);
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
	async_forward_1(icall, exch, INTERFACE_DDF_CLIENT, handle, IPC_FF_NONE);
	async_exchange_end(exch);

cleanup:
	if (dev != NULL)
		dev_del_ref(dev);

	if (fun != NULL)
		fun_del_ref(fun);
}

static void devman_connection_parent(ipc_call_t *icall, void *arg)
{
	devman_handle_t handle = ipc_get_arg2(icall);
	dev_node_t *dev = NULL;

	fun_node_t *fun = find_fun_node(&device_tree, handle);
	if (fun == NULL) {
		dev = find_dev_node(&device_tree, handle);
	} else {
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
		async_answer_0(icall, ENOENT);
		goto cleanup;
	}

	driver_t *driver = NULL;

	fibril_rwlock_read_lock(&device_tree.rwlock);

	/* Connect to parent function of a device (or device function). */
	if (dev->pfun->dev != NULL)
		driver = dev->pfun->dev->drv;

	devman_handle_t fun_handle = dev->pfun->handle;

	fibril_rwlock_read_unlock(&device_tree.rwlock);

	if (driver == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "IPC forwarding refused - "
		    "the device %" PRIun " is not in usable state.", handle);
		async_answer_0(icall, ENOENT);
		goto cleanup;
	}

	if (!driver->sess) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Could not forward to driver `%s'.", driver->name);
		async_answer_0(icall, EINVAL);
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
	async_forward_1(icall, exch, INTERFACE_DDF_DRIVER, fun_handle, IPC_FF_NONE);
	async_exchange_end(exch);

cleanup:
	if (dev != NULL)
		dev_del_ref(dev);

	if (fun != NULL)
		fun_del_ref(fun);
}

static void devman_forward(ipc_call_t *icall, void *arg)
{
	iface_t iface = ipc_get_arg1(icall);
	service_id_t service_id = ipc_get_arg2(icall);

	fun_node_t *fun = find_loc_tree_function(&device_tree, service_id);

	fibril_rwlock_read_lock(&device_tree.rwlock);

	if ((fun == NULL) || (fun->dev == NULL) || (fun->dev->drv == NULL)) {
		log_msg(LOG_DEFAULT, LVL_WARN, "devman_forward(): function "
		    "not found.\n");
		fibril_rwlock_read_unlock(&device_tree.rwlock);
		async_answer_0(icall, ENOENT);
		return;
	}

	dev_node_t *dev = fun->dev;
	driver_t *driver = dev->drv;
	devman_handle_t handle = fun->handle;

	fibril_rwlock_read_unlock(&device_tree.rwlock);

	async_exch_t *exch = async_exchange_begin(driver->sess);
	async_forward_1(icall, exch, iface, handle, IPC_FF_NONE);
	async_exchange_end(exch);

	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "Forwarding service request for `%s' function to driver `%s'.",
	    fun->pathname, driver->name);

	fun_del_ref(fun);
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
	errno_t rc;

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
	rc = loc_server_register(NAME, &devman_srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_FATAL, "Error registering devman server.");
		return false;
	}

	return true;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS Device Manager\n", NAME);

	errno_t rc = log_init(NAME);
	if (rc != EOK) {
		printf("%s: Error initializing logging subsystem: %s\n", NAME, str_error(rc));
		return rc;
	}

	/* Set handlers for incoming connections. */
	async_set_client_data_constructor(devman_client_data_create);
	async_set_client_data_destructor(devman_client_data_destroy);

	async_set_fallback_port_handler(devman_forward, NULL);

	if (!devman_init()) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error while initializing service.");
		return -1;
	}

	/* Register device manager at naming service. */
	rc = service_register(SERVICE_DEVMAN, INTERFACE_DDF_DRIVER,
	    devman_connection_driver, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering driver port: %s", str_error(rc));
		return rc;
	}

	rc = service_register(SERVICE_DEVMAN, INTERFACE_DDF_CLIENT,
	    devman_connection_client, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering client port: %s", str_error(rc));
		return rc;
	}

	rc = service_register(SERVICE_DEVMAN, INTERFACE_DEVMAN_DEVICE,
	    devman_connection_device, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering device port: %s", str_error(rc));
		return rc;
	}

	rc = service_register(SERVICE_DEVMAN, INTERFACE_DEVMAN_PARENT,
	    devman_connection_parent, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering parent port: %s", str_error(rc));
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
