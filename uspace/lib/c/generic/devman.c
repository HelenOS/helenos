/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2013 Jiri Svoboda
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
#include <stdlib.h>

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

static async_sess_t *devman_session_blocking(iface_t iface)
{
	switch (iface) {
	case INTERFACE_DDF_DRIVER:
		fibril_mutex_lock(&devman_driver_block_mutex);

		while (devman_driver_block_sess == NULL) {
			clone_session(&devman_driver_mutex, devman_driver_sess,
			    &devman_driver_block_sess);

			if (devman_driver_block_sess == NULL)
				devman_driver_block_sess =
				    service_connect_blocking(SERVICE_DEVMAN,
				    INTERFACE_DDF_DRIVER, 0);
		}

		fibril_mutex_unlock(&devman_driver_block_mutex);

		clone_session(&devman_driver_mutex, devman_driver_block_sess,
		    &devman_driver_sess);

		return devman_driver_block_sess;
	case INTERFACE_DDF_CLIENT:
		fibril_mutex_lock(&devman_client_block_mutex);

		while (devman_client_block_sess == NULL) {
			clone_session(&devman_client_mutex, devman_client_sess,
			    &devman_client_block_sess);

			if (devman_client_block_sess == NULL)
				devman_client_block_sess =
				    service_connect_blocking(SERVICE_DEVMAN,
				    INTERFACE_DDF_CLIENT, 0);
		}

		fibril_mutex_unlock(&devman_client_block_mutex);

		clone_session(&devman_client_mutex, devman_client_block_sess,
		    &devman_client_sess);

		return devman_client_block_sess;
	default:
		return NULL;
	}
}

/** Start an async exchange on the devman session (blocking).
 *
 * @param iface Device manager interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devman_exchange_begin_blocking(iface_t iface)
{
	async_sess_t *sess = devman_session_blocking(iface);
	if (!sess)
		return NULL;
	return async_exchange_begin(sess);
}

static async_sess_t *devman_session(iface_t iface)
{
	switch (iface) {
	case INTERFACE_DDF_DRIVER:
		fibril_mutex_lock(&devman_driver_mutex);

		if (devman_driver_sess == NULL)
			devman_driver_sess =
			    service_connect(SERVICE_DEVMAN,
			    INTERFACE_DDF_DRIVER, 0);

		fibril_mutex_unlock(&devman_driver_mutex);

		if (devman_driver_sess == NULL)
			return NULL;

		return devman_driver_sess;
	case INTERFACE_DDF_CLIENT:
		fibril_mutex_lock(&devman_client_mutex);

		if (devman_client_sess == NULL)
			devman_client_sess =
			    service_connect(SERVICE_DEVMAN,
			    INTERFACE_DDF_CLIENT, 0);

		fibril_mutex_unlock(&devman_client_mutex);

		if (devman_client_sess == NULL)
			return NULL;

		return devman_client_sess;
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
async_exch_t *devman_exchange_begin(iface_t iface)
{
	async_sess_t *sess = devman_session(iface);
	if (!sess)
		return NULL;
	return async_exchange_begin(sess);
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
errno_t devman_driver_register(const char *name)
{
	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_DRIVER);

	async_call_t call;
	async_call_data_t write, connect;

	async_call_begin(&call, sess, DEVMAN_DRIVER_REGISTER, 0, 0, 0, 0);
	async_call_write(&call, &write, name, str_size(name), NULL);
	async_call_connect_to_me(&call, &connect, 0, 0, 0);

	return async_call_finish(&call);
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
 * @return EOK on success or an error code.
 *
 */
errno_t devman_add_function(const char *name, fun_type_t ftype,
    match_id_list_t *match_ids, devman_handle_t devh, devman_handle_t *funh)
{
	unsigned long match_count = list_count(&match_ids->ids);

	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_DRIVER);

	async_call_t call;
	async_call_data_t write1, method_n, write_n;

	async_call_begin(&call, sess, DEVMAN_ADD_FUNCTION,
	    (sysarg_t) ftype, devh, match_count, 0);

	async_call_write(&call, &write1, name, str_size(name), NULL);

	list_foreach(match_ids->ids, link, match_id_t, match_id) {
		async_call_method(&call, &method_n, DEVMAN_ADD_MATCH_ID,
		    match_id->score, 0, 0, 0);

		async_call_write(&call, &write_n,
		    match_id->id, str_size(match_id->id), NULL);

		/* Wait so that we can recycle `method_n` and `write_n`. */
		(void) async_call_wait(&call);
	}

	errno_t rc = async_call_finish(&call);

	if (funh) {
		if (rc == EOK) {
			*funh = (devman_handle_t)
			    IPC_GET_ARG1(call.initial.answer);
		} else {
			*funh = -1;
		}
	}

	return rc;
}

errno_t devman_add_device_to_category(devman_handle_t devman_handle,
    const char *cat_name)
{
	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_DRIVER);

	async_call_t call;
	async_call_data_t write;

	async_call_begin(&call, sess, DEVMAN_ADD_DEVICE_TO_CATEGORY, devman_handle, 0, 0, 0);
	async_call_write(&call, &write, cat_name, str_size(cat_name), NULL);
	return async_call_finish(&call);
}

async_sess_t *devman_device_connect(devman_handle_t handle, unsigned int flags)
{
	async_sess_t *sess;

	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(SERVICE_DEVMAN,
		    INTERFACE_DEVMAN_DEVICE, handle);
	else
		sess = service_connect(SERVICE_DEVMAN,
		    INTERFACE_DEVMAN_DEVICE, handle);

	return sess;
}

/** Remove function from device.
 *
 * Request devman to remove function owned by this driver task.
 * @param funh      Devman handle of the function
 *
 * @return EOK on success or an error code.
 */
errno_t devman_remove_function(devman_handle_t funh)
{
	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_DRIVER);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_REMOVE_FUNCTION, funh, 0, 0, 0);
	return async_call_finish(&call);
}

errno_t devman_drv_fun_online(devman_handle_t funh)
{
	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_DRIVER);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_DRV_FUN_ONLINE, funh, 0, 0, 0);
	return async_call_finish(&call);
}

errno_t devman_drv_fun_offline(devman_handle_t funh)
{
	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_DRIVER);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_DRV_FUN_OFFLINE, funh, 0, 0, 0);
	return async_call_finish(&call);
}

async_sess_t *devman_parent_device_connect(devman_handle_t handle,
    unsigned int flags)
{
	async_sess_t *sess;

	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(SERVICE_DEVMAN,
		    INTERFACE_DEVMAN_PARENT, handle);
	else
		sess = service_connect_blocking(SERVICE_DEVMAN,
		    INTERFACE_DEVMAN_PARENT, handle);

	return sess;
}

errno_t devman_fun_get_handle(const char *pathname, devman_handle_t *handle,
    unsigned int flags)
{
	async_sess_t *sess;
	async_call_t call;
	async_call_data_t write;

	if (flags & IPC_FLAG_BLOCKING)
		sess = devman_session_blocking(INTERFACE_DDF_CLIENT);
	else
		sess = devman_session(INTERFACE_DDF_CLIENT);

	async_call_begin(&call, sess, DEVMAN_DEVICE_GET_HANDLE, flags, 0, 0, 0);
	async_call_write(&call, &write, pathname, str_size(pathname), NULL);
	errno_t rc = async_call_finish(&call);

	if (rc != EOK) {
		if (handle != NULL)
			*handle = (devman_handle_t) -1;

		return rc;
	}

	if (handle != NULL)
		*handle = (devman_handle_t) IPC_GET_ARG1(call.initial.answer);

	return EOK;
}

static errno_t devman_get_str_internal(sysarg_t method, sysarg_t arg1,
    sysarg_t arg2, sysarg_t *r1, char *buf, size_t buf_size)
{
	async_call_t call;
	async_call_data_t read;
	size_t act_size;

	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_CLIENT);

	async_call_begin(&call, sess, method, arg1, arg2, 0, 0);
	async_call_read(&call, &read, buf, buf_size - 1, &act_size);
	errno_t rc = async_call_finish(&call);
	if (rc != EOK)
		return rc;

	if (r1 != NULL)
		*r1 = IPC_GET_ARG1(call.initial.answer);

	assert(act_size <= buf_size - 1);
	buf[act_size] = '\0';
	return EOK;
}

errno_t devman_fun_get_path(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_FUN_GET_PATH, handle, 0, NULL,
	    buf, buf_size);
}

errno_t devman_fun_get_match_id(devman_handle_t handle, size_t index, char *buf,
    size_t buf_size, unsigned int *rscore)
{
	errno_t rc;
	sysarg_t score = 0;

	rc = devman_get_str_internal(DEVMAN_FUN_GET_MATCH_ID, handle, index,
	    &score, buf, buf_size);
	if (rc != EOK)
		return rc;

	*rscore = score;
	return rc;
}

errno_t devman_fun_get_name(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_FUN_GET_NAME, handle, 0, NULL,
	    buf, buf_size);
}

errno_t devman_fun_get_driver_name(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_FUN_GET_DRIVER_NAME, handle, 0,
	    NULL, buf, buf_size);
}

errno_t devman_fun_online(devman_handle_t funh)
{
	async_sess_t *sess = devman_session(INTERFACE_DDF_CLIENT);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_FUN_ONLINE, funh, 0, 0, 0);
	return async_call_finish(&call);
}

errno_t devman_fun_offline(devman_handle_t funh)
{
	async_sess_t *sess = devman_session(INTERFACE_DDF_CLIENT);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_FUN_OFFLINE, funh, 0, 0, 0);
	return async_call_finish(&call);
}

static errno_t devman_get_handles_once(sysarg_t method, sysarg_t arg1,
    devman_handle_t *handle_buf, size_t buf_size, size_t *act_size)
{
	async_sess_t *sess = devman_session_blocking(INTERFACE_DDF_CLIENT);

	async_call_t call;
	async_call_data_t read;

	async_call_begin(&call, sess, method, arg1, 0, 0, 0);
	async_call_read(&call, &read, handle_buf, buf_size, NULL);
	errno_t rc = async_call_finish(&call);
	if (rc != EOK)
		return rc;

	*act_size = IPC_GET_ARG1(call.initial.answer);
	return EOK;
}

/** Get list of handles.
 *
 * Returns an allocated array of handles.
 *
 * @param method	IPC method
 * @param arg1		IPC argument 1
 * @param data		Place to store pointer to array of handles
 * @param count		Place to store number of handles
 * @return 		EOK on success or an error code
 */
static errno_t devman_get_handles_internal(sysarg_t method, sysarg_t arg1,
    devman_handle_t **data, size_t *count)
{
	devman_handle_t *handles;
	size_t act_size;
	size_t alloc_size;
	errno_t rc;

	*data = NULL;
	act_size = 0;	/* silence warning */

	rc = devman_get_handles_once(method, arg1, NULL, 0,
	    &act_size);
	if (rc != EOK)
		return rc;

	alloc_size = act_size;
	handles = malloc(alloc_size);
	if (handles == NULL)
		return ENOMEM;

	while (true) {
		rc = devman_get_handles_once(method, arg1, handles, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size *= 2;
		free(handles);

		handles = malloc(alloc_size);
		if (handles == NULL)
			return ENOMEM;
	}

	*count = act_size / sizeof(devman_handle_t);
	*data = handles;
	return EOK;
}

errno_t devman_fun_get_child(devman_handle_t funh, devman_handle_t *devh)
{
	async_sess_t *sess = devman_session(INTERFACE_DDF_CLIENT);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_FUN_GET_CHILD, funh, 0, 0, 0);
	errno_t rc = async_call_finish(&call);
	if (rc != EOK)
		return rc;

	*devh = (devman_handle_t) IPC_GET_ARG1(call.initial.answer);
	return EOK;
}

errno_t devman_dev_get_functions(devman_handle_t devh, devman_handle_t **funcs,
    size_t *count)
{
	return devman_get_handles_internal(DEVMAN_DEV_GET_FUNCTIONS,
	    devh, funcs, count);
}

errno_t devman_dev_get_parent(devman_handle_t devh, devman_handle_t *funh)
{
	async_sess_t *sess = devman_session(INTERFACE_DDF_CLIENT);

	async_call_t call;
	async_call_begin(&call, sess, DEVMAN_DEV_GET_PARENT, devh, 0, 0, 0);
	errno_t rc = async_call_finish(&call);
	if (rc != EOK)
		return rc;

	*funh = (devman_handle_t) IPC_GET_ARG1(call.initial.answer);
	return EOK;
}

errno_t devman_fun_sid_to_handle(service_id_t sid, devman_handle_t *handle)
{
	async_exch_t *exch = devman_exchange_begin(INTERFACE_DDF_CLIENT);
	if (exch == NULL)
		return ENOMEM;

	errno_t retval = async_req_1_1(exch, DEVMAN_FUN_SID_TO_HANDLE,
	    sid, handle);

	devman_exchange_end(exch);
	return retval;
}

errno_t devman_get_drivers(devman_handle_t **drvs,
    size_t *count)
{
	return devman_get_handles_internal(DEVMAN_GET_DRIVERS, 0, drvs, count);
}

errno_t devman_driver_get_devices(devman_handle_t drvh, devman_handle_t **devs,
    size_t *count)
{
	return devman_get_handles_internal(DEVMAN_DRIVER_GET_DEVICES,
	    drvh, devs, count);
}

errno_t devman_driver_get_handle(const char *drvname, devman_handle_t *handle)
{
	async_exch_t *exch;

	exch = devman_exchange_begin(INTERFACE_DDF_CLIENT);
	if (exch == NULL)
		return ENOMEM;

	ipc_call_t answer;
	aid_t req = async_send_0(exch, DEVMAN_DRIVER_GET_HANDLE, &answer);
	errno_t retval = async_data_write_start(exch, drvname,
	    str_size(drvname));

	devman_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
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

errno_t devman_driver_get_match_id(devman_handle_t handle, size_t index, char *buf,
    size_t buf_size, unsigned int *rscore)
{
	errno_t rc;
	sysarg_t score = 0;

	rc = devman_get_str_internal(DEVMAN_DRIVER_GET_MATCH_ID, handle, index,
	    &score, buf, buf_size);
	if (rc != EOK)
		return rc;

	*rscore = score;
	return rc;
}

errno_t devman_driver_get_name(devman_handle_t handle, char *buf, size_t buf_size)
{
	return devman_get_str_internal(DEVMAN_DRIVER_GET_NAME, handle, 0, NULL,
	    buf, buf_size);
}

errno_t devman_driver_get_state(devman_handle_t drvh, driver_state_t *rstate)
{
	sysarg_t state;
	async_exch_t *exch = devman_exchange_begin(INTERFACE_DDF_CLIENT);
	if (exch == NULL)
		return ENOMEM;

	errno_t rc = async_req_1_1(exch, DEVMAN_DRIVER_GET_STATE, drvh,
	    &state);

	devman_exchange_end(exch);
	if (rc != EOK)
		return rc;

	*rstate = state;
	return rc;
}

errno_t devman_driver_load(devman_handle_t drvh)
{
	async_exch_t *exch = devman_exchange_begin(INTERFACE_DDF_CLIENT);
	if (exch == NULL)
		return ENOMEM;

	errno_t rc = async_req_1_0(exch, DEVMAN_DRIVER_LOAD, drvh);

	devman_exchange_end(exch);
	return rc;
}

errno_t devman_driver_unload(devman_handle_t drvh)
{
	async_exch_t *exch = devman_exchange_begin(INTERFACE_DDF_CLIENT);
	if (exch == NULL)
		return ENOMEM;

	errno_t rc = async_req_1_0(exch, DEVMAN_DRIVER_UNLOAD, drvh);

	devman_exchange_end(exch);
	return rc;
}

/** @}
 */
