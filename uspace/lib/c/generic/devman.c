/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <adt/list.h>
#include <str.h>
#include <ipc/services.h>
#include <ns.h>
#include <ipc/devman.h>
#include <devman.h>
#include <fibril_synch.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>

static FIBRIL_MUTEX_INITIALIZE(devman_driver_block_mutex);
static FIBRIL_MUTEX_INITIALIZE(devman_client_block_mutex);

static FIBRIL_MUTEX_INITIALIZE(devman_driver_mutex);
static FIBRIL_MUTEX_INITIALIZE(devman_client_mutex);

static async_sess_t *devman_driver_block_sess = NULL;
static async_sess_t *devman_client_block_sess = NULL;

static async_sess_t *devman_driver_sess = NULL;
static async_sess_t *devman_client_sess = NULL;

static void clone_session(fibril_mutex_t *mtx, async_sess_t *src,
    async_sess_t **dst)
{
	fibril_mutex_lock(mtx);
	
	if ((*dst == NULL) && (src != NULL))
		*dst = src;
	
	fibril_mutex_unlock(mtx);
}

/** Start an async exchange on the devman session (blocking).
 *
 * @param iface Device manager interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devman_exchange_begin_blocking(devman_interface_t iface)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		fibril_mutex_lock(&devman_driver_block_mutex);
		
		while (devman_driver_block_sess == NULL) {
			clone_session(&devman_driver_mutex, devman_driver_sess,
			    &devman_driver_block_sess);
			
			if (devman_driver_block_sess == NULL)
				devman_driver_block_sess =
				    service_connect_blocking(EXCHANGE_SERIALIZE,
				    SERVICE_DEVMAN, DEVMAN_DRIVER, 0);
		}
		
		fibril_mutex_unlock(&devman_driver_block_mutex);
		
		clone_session(&devman_driver_mutex, devman_driver_block_sess,
		    &devman_driver_sess);
		
		return async_exchange_begin(devman_driver_block_sess);
	case DEVMAN_CLIENT:
		fibril_mutex_lock(&devman_client_block_mutex);
		
		while (devman_client_block_sess == NULL) {
			clone_session(&devman_client_mutex, devman_client_sess,
			    &devman_client_block_sess);
			
			if (devman_client_block_sess == NULL)
				devman_client_block_sess =
				    service_connect_blocking(EXCHANGE_SERIALIZE,
				    SERVICE_DEVMAN, DEVMAN_CLIENT, 0);
		}
		
		fibril_mutex_unlock(&devman_client_block_mutex);
		
		clone_session(&devman_client_mutex, devman_client_block_sess,
		    &devman_client_sess);
		
		return async_exchange_begin(devman_client_block_sess);
	default:
		return NULL;
	}
}

/** Start an async exchange on the devman session.
 *
 * @param iface Device manager interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devman_exchange_begin(devman_interface_t iface)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		fibril_mutex_lock(&devman_driver_mutex);
		
		if (devman_driver_sess == NULL)
			devman_driver_sess =
			    service_connect(EXCHANGE_SERIALIZE, SERVICE_DEVMAN,
			    DEVMAN_DRIVER, 0);
		
		fibril_mutex_unlock(&devman_driver_mutex);
		
		if (devman_driver_sess == NULL)
			return NULL;
		
		return async_exchange_begin(devman_driver_sess);
	case DEVMAN_CLIENT:
		fibril_mutex_lock(&devman_client_mutex);
		
		if (devman_client_sess == NULL)
			devman_client_sess =
			    service_connect(EXCHANGE_SERIALIZE, SERVICE_DEVMAN,
			    DEVMAN_CLIENT, 0);
		
		fibril_mutex_unlock(&devman_client_mutex);
		
		if (devman_client_sess == NULL)
			return NULL;
		
		return async_exchange_begin(devman_client_sess);
	default:
		return NULL;
	}
}

/** Finish an async exchange on the devman session.
 *
 * @param exch Exchange to be finished.
 *
 */
void devman_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Register running driver with device manager. */
int devman_driver_register(const char *name, async_client_conn_t conn)
{
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAN_DRIVER_REGISTER, 0, 0, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	
	devman_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_set_client_connection(conn);
	
	exch = devman_exchange_begin(DEVMAN_DRIVER);
	async_connect_to_me(exch, 0, 0, 0, NULL, NULL);
	devman_exchange_end(exch);
	
	async_wait_for(req, &retval);
	return retval;
}

/** Add function to a device.
 *
 * Request devman to add a new function to the specified device owned by
 * this driver task.
 *
 * @param name      Name of the new function
 * @param ftype     Function type, fun_inner or fun_exposed
 * @param match_ids Match IDs (should be empty for fun_exposed)
 * @param devh      Devman handle of the device
 * @param funh      Place to store handle of the new function
 *
 * @return EOK on success or negative error code.
 *
 */
int devman_add_function(const char *name, fun_type_t ftype,
    match_id_list_t *match_ids, devman_handle_t devh, devman_handle_t *funh)
{
	int match_count = list_count(&match_ids->ids);
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_3(exch, DEVMAN_ADD_FUNCTION, (sysarg_t) ftype,
	    devh, match_count, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	if (retval != EOK) {
		devman_exchange_end(exch);
		async_wait_for(req, NULL);
		return retval;
	}
	
	match_id_t *match_id = NULL;
	
	list_foreach(match_ids->ids, link) {
		match_id = list_get_instance(link, match_id_t, link);
		
		ipc_call_t answer2;
		aid_t req2 = async_send_1(exch, DEVMAN_ADD_MATCH_ID,
		    match_id->score, &answer2);
		retval = async_data_write_start(exch, match_id->id,
		    str_size(match_id->id));
		if (retval != EOK) {
			devman_exchange_end(exch);
			async_wait_for(req2, NULL);
			async_wait_for(req, NULL);
			return retval;
		}
		
		async_wait_for(req2, &retval);
		if (retval != EOK) {
			devman_exchange_end(exch);
			async_wait_for(req, NULL);
			return retval;
		}
	}
	
	devman_exchange_end(exch);
	
	async_wait_for(req, &retval);
	if (retval == EOK) {
		if (funh != NULL)
			*funh = (int) IPC_GET_ARG1(answer);
	} else {
		if (funh != NULL)
			*funh = -1;
	}
	
	return retval;
}

int devman_add_device_to_category(devman_handle_t devman_handle,
    const char *cat_name)
{
	async_exch_t *exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEVMAN_ADD_DEVICE_TO_CATEGORY,
	    devman_handle, &answer);
	sysarg_t retval = async_data_write_start(exch, cat_name,
	    str_size(cat_name));
	
	devman_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_wait_for(req, &retval);
	return retval;
}

async_sess_t *devman_device_connect(exch_mgmt_t mgmt, devman_handle_t handle,
    unsigned int flags)
{
	async_sess_t *sess;
	
	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_DEVICE, handle);
	else
		sess = service_connect(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_DEVICE, handle);
	
	return sess;
}

/** Remove function from device.
 *
 * Request devman to remove function owned by this driver task.
 * @param funh      Devman handle of the function
 *
 * @return EOK on success or negative error code.
 */
int devman_remove_function(devman_handle_t funh)
{
	async_exch_t *exch;
	sysarg_t retval;
	
	exch = devman_exchange_begin_blocking(DEVMAN_DRIVER);
	retval = async_req_1_0(exch, DEVMAN_REMOVE_FUNCTION, (sysarg_t) funh);
	devman_exchange_end(exch);
	
	return (int) retval;
}

async_sess_t *devman_parent_device_connect(exch_mgmt_t mgmt,
    devman_handle_t handle, unsigned int flags)
{
	async_sess_t *sess;
	
	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	else
		sess = service_connect(mgmt, SERVICE_DEVMAN,
			    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	
	return sess;
}

int devman_device_get_handle(const char *pathname, devman_handle_t *handle,
    unsigned int flags)
{
	async_exch_t *exch;
	
	if (flags & IPC_FLAG_BLOCKING)
		exch = devman_exchange_begin_blocking(DEVMAN_CLIENT);
	else {
		exch = devman_exchange_begin(DEVMAN_CLIENT);
		if (exch == NULL)
			return ENOMEM;
	}
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAN_DEVICE_GET_HANDLE, flags, 0,
	    &answer);
	sysarg_t retval = async_data_write_start(exch, pathname,
	    str_size(pathname));
	
	devman_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (devman_handle_t) -1;
		
		return retval;
	}
	
	if (handle != NULL)
		*handle = (devman_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

int devman_get_device_path(devman_handle_t handle, char *path, size_t path_size)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_CLIENT);
	if (exch == NULL)
		return ENOMEM;
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, DEVMAN_DEVICE_GET_DEVICE_PATH,
	    handle, &answer);
	
	ipc_call_t data_request_call;
	aid_t data_request = async_data_read(exch, path, path_size,
	    &data_request_call);
	
	devman_exchange_end(exch);
	
	if (data_request == 0) {
		async_wait_for(req, NULL);
		return ENOMEM;
	}
	
	sysarg_t data_request_rc;
	async_wait_for(data_request, &data_request_rc);
	
	sysarg_t opening_request_rc;
	async_wait_for(req, &opening_request_rc);
	
	if (data_request_rc != EOK) {
		/* Prefer the return code of the opening request. */
		if (opening_request_rc != EOK)
			return (int) opening_request_rc;
		else
			return (int) data_request_rc;
	}
	
	if (opening_request_rc != EOK)
		return (int) opening_request_rc;
	
	/* To be on the safe-side. */
	path[path_size - 1] = 0;
	size_t transferred_size = IPC_GET_ARG2(data_request_call);
	if (transferred_size >= path_size)
		return ELIMIT;
	
	/* Terminate the string (trailing 0 not send over IPC). */
	path[transferred_size] = 0;
	return EOK;
}

int devman_fun_sid_to_handle(service_id_t sid, devman_handle_t *handle)
{
	async_exch_t *exch = devman_exchange_begin(DEVMAN_CLIENT);
	if (exch == NULL)
		return ENOMEM;
	
	sysarg_t retval = async_req_1_1(exch, DEVMAN_FUN_SID_TO_HANDLE,
	    sid, handle);
	
	devman_exchange_end(exch);
	return (int) retval;
}

/** @}
 */
