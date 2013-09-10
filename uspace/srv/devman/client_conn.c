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
#include <ipc/devman.h>

#include "client_conn.h"
#include "dev.h"
#include "devman.h"
#include "driver.h"
#include "fun.h"
#include "loc.h"
#include "main.h"

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

/** Get list of all registered drivers. */
static void devman_get_drivers(ipc_callid_t iid, ipc_call_t *icall)
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
	
	devman_handle_t *hdl_buf = (devman_handle_t *) malloc(size);
	if (hdl_buf == NULL) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	rc = driver_get_list(&drivers_list, hdl_buf, size, &act_size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	sysarg_t retval = async_data_read_finalize(callid, hdl_buf, size);
	free(hdl_buf);
	
	async_answer_1(iid, retval, act_size);
}

/** Get driver name. */
static void devman_driver_get_name(ipc_callid_t iid, ipc_call_t *icall)
{
	devman_handle_t handle = IPC_GET_ARG1(*icall);

	driver_t *drv = driver_find(&drivers_list, handle);
	if (drv == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	ipc_callid_t data_callid;
	size_t data_len;
	if (!async_data_read_receive(&data_callid, &data_len)) {
		async_answer_0(iid, EINVAL);
		return;
	}

	void *buffer = malloc(data_len);
	if (buffer == NULL) {
		async_answer_0(data_callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	fibril_mutex_lock(&drv->driver_mutex);

	size_t sent_length = str_size(drv->name);
	if (sent_length > data_len) {
		sent_length = data_len;
	}

	async_data_read_finalize(data_callid, drv->name, sent_length);
	async_answer_0(iid, EOK);

	fibril_mutex_unlock(&drv->driver_mutex);

	free(buffer);
}

/** Function for handling connections from a client to the device manager. */
void devman_connection_client(ipc_callid_t iid, ipc_call_t *icall)
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
		case DEVMAN_GET_DRIVERS:
			devman_get_drivers(callid, &call);
			break;
		case DEVMAN_DRIVER_GET_NAME:
			devman_driver_get_name(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOENT);
		}
	}
}


/** @}
 */
