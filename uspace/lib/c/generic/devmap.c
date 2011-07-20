/*
 * Copyright (c) 2007 Josef Cejka
 * Copyright (c) 2009 Jiri Svoboda
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

#include <str.h>
#include <ipc/services.h>
#include <ns.h>
#include <ipc/devmap.h>
#include <devmap.h>
#include <fibril_synch.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>

static FIBRIL_MUTEX_INITIALIZE(devmap_driver_block_mutex);
static FIBRIL_MUTEX_INITIALIZE(devmap_client_block_mutex);

static FIBRIL_MUTEX_INITIALIZE(devmap_driver_mutex);
static FIBRIL_MUTEX_INITIALIZE(devmap_client_mutex);

static async_sess_t *devmap_driver_block_sess = NULL;
static async_sess_t *devmap_client_block_sess = NULL;

static async_sess_t *devmap_driver_sess = NULL;
static async_sess_t *devmap_client_sess = NULL;

static void clone_session(fibril_mutex_t *mtx, async_sess_t *src,
    async_sess_t **dst)
{
	fibril_mutex_lock(mtx);
	
	if ((*dst == NULL) && (src != NULL))
		*dst = src;
	
	fibril_mutex_unlock(mtx);
}

/** Start an async exchange on the devmap session (blocking).
 *
 * @param iface Device mapper interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devmap_exchange_begin_blocking(devmap_interface_t iface)
{
	switch (iface) {
	case DEVMAP_DRIVER:
		fibril_mutex_lock(&devmap_driver_block_mutex);
		
		while (devmap_driver_block_sess == NULL) {
			clone_session(&devmap_driver_mutex, devmap_driver_sess,
			    &devmap_driver_block_sess);
			
			if (devmap_driver_block_sess == NULL)
				devmap_driver_block_sess =
				    service_connect_blocking(EXCHANGE_SERIALIZE,
				    SERVICE_DEVMAP, DEVMAP_DRIVER, 0);
		}
		
		fibril_mutex_unlock(&devmap_driver_block_mutex);
		
		clone_session(&devmap_driver_mutex, devmap_driver_block_sess,
		    &devmap_driver_sess);
		
		return async_exchange_begin(devmap_driver_block_sess);
	case DEVMAP_CLIENT:
		fibril_mutex_lock(&devmap_client_block_mutex);
		
		while (devmap_client_block_sess == NULL) {
			clone_session(&devmap_client_mutex, devmap_client_sess,
			    &devmap_client_block_sess);
			
			if (devmap_client_block_sess == NULL)
				devmap_client_block_sess =
				    service_connect_blocking(EXCHANGE_SERIALIZE,
				    SERVICE_DEVMAP, DEVMAP_CLIENT, 0);
		}
		
		fibril_mutex_unlock(&devmap_client_block_mutex);
		
		clone_session(&devmap_client_mutex, devmap_client_block_sess,
		    &devmap_client_sess);
		
		return async_exchange_begin(devmap_client_block_sess);
	default:
		return NULL;
	}
}

/** Start an async exchange on the devmap session.
 *
 * @param iface Device mapper interface to choose
 *
 * @return New exchange.
 *
 */
async_exch_t *devmap_exchange_begin(devmap_interface_t iface)
{
	switch (iface) {
	case DEVMAP_DRIVER:
		fibril_mutex_lock(&devmap_driver_mutex);
		
		if (devmap_driver_sess == NULL)
			devmap_driver_sess =
			    service_connect(EXCHANGE_SERIALIZE, SERVICE_DEVMAP,
			    DEVMAP_DRIVER, 0);
		
		fibril_mutex_unlock(&devmap_driver_mutex);
		
		if (devmap_driver_sess == NULL)
			return NULL;
		
		return async_exchange_begin(devmap_driver_sess);
	case DEVMAP_CLIENT:
		fibril_mutex_lock(&devmap_client_mutex);
		
		if (devmap_client_sess == NULL)
			devmap_client_sess =
			    service_connect(EXCHANGE_SERIALIZE, SERVICE_DEVMAP,
			    DEVMAP_CLIENT, 0);
		
		fibril_mutex_unlock(&devmap_client_mutex);
		
		if (devmap_client_sess == NULL)
			return NULL;
		
		return async_exchange_begin(devmap_client_sess);
	default:
		return NULL;
	}
}

/** Finish an async exchange on the devmap session.
 *
 * @param exch Exchange to be finished.
 *
 */
void devmap_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

/** Register new driver with devmap. */
int devmap_driver_register(const char *name, async_client_conn_t conn)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAP_DRIVER_REGISTER, 0, 0, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	
	devmap_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_set_client_connection(conn);
	
	exch = devmap_exchange_begin(DEVMAP_DRIVER);
	async_connect_to_me(exch, 0, 0, 0, NULL, NULL);
	devmap_exchange_end(exch);
	
	async_wait_for(req, &retval);
	return retval;
}

/** Register new device.
 *
 * The @p interface is used when forwarding connection to the driver.
 * If not 0, the first argument is the interface and the second argument
 * is the devmap handle of the device.
 *
 * When the interface is zero (default), the first argument is directly
 * the handle (to ensure backward compatibility).
 *
 * @param      fqdn      Fully qualified device name.
 * @param[out] handle    Handle to the created instance of device.
 * @param      interface Interface when forwarding.
 *
 */
int devmap_device_register_with_iface(const char *fqdn,
    devmap_handle_t *handle, sysarg_t interface)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_DRIVER);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAP_DEVICE_REGISTER, interface, 0,
	    &answer);
	sysarg_t retval = async_data_write_start(exch, fqdn, str_size(fqdn));
	
	devmap_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = -1;
		
		return retval;
	}
	
	if (handle != NULL)
		*handle = (devmap_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

/** Register new device.
 *
 * @param fqdn   Fully qualified device name.
 * @param handle Output: Handle to the created instance of device.
 *
 */
int devmap_device_register(const char *fqdn, devmap_handle_t *handle)
{
	return devmap_device_register_with_iface(fqdn, handle, 0);
}

int devmap_device_get_handle(const char *fqdn, devmap_handle_t *handle,
    unsigned int flags)
{
	async_exch_t *exch;
	
	if (flags & IPC_FLAG_BLOCKING)
		exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	else {
		exch = devmap_exchange_begin(DEVMAP_CLIENT);
		if (exch == NULL)
			return errno;
	}
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAP_DEVICE_GET_HANDLE, flags, 0,
	    &answer);
	sysarg_t retval = async_data_write_start(exch, fqdn, str_size(fqdn));
	
	devmap_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (devmap_handle_t) -1;
		
		return retval;
	}
	
	if (handle != NULL)
		*handle = (devmap_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

int devmap_namespace_get_handle(const char *name, devmap_handle_t *handle,
    unsigned int flags)
{
	async_exch_t *exch;
	
	if (flags & IPC_FLAG_BLOCKING)
		exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	else {
		exch = devmap_exchange_begin(DEVMAP_CLIENT);
		if (exch == NULL)
			return errno;
	}
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DEVMAP_NAMESPACE_GET_HANDLE, flags, 0,
	    &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	
	devmap_exchange_end(exch);
	
	if (retval != EOK) {
		async_wait_for(req, NULL);
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (devmap_handle_t) -1;
		
		return retval;
	}
	
	if (handle != NULL)
		*handle = (devmap_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

devmap_handle_type_t devmap_handle_probe(devmap_handle_t handle)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	
	sysarg_t type;
	int retval = async_req_1_1(exch, DEVMAP_HANDLE_PROBE, handle, &type);
	
	devmap_exchange_end(exch);
	
	if (retval != EOK)
		return DEV_HANDLE_NONE;
	
	return (devmap_handle_type_t) type;
}

async_sess_t *devmap_device_connect(exch_mgmt_t mgmt, devmap_handle_t handle,
    unsigned int flags)
{
	async_sess_t *sess;
	
	if (flags & IPC_FLAG_BLOCKING)
		sess = service_connect_blocking(mgmt, SERVICE_DEVMAP,
		    DEVMAP_CONNECT_TO_DEVICE, handle);
	else
		sess = service_connect(mgmt, SERVICE_DEVMAP,
		    DEVMAP_CONNECT_TO_DEVICE, handle);
	
	return sess;
}

int devmap_null_create(void)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	
	sysarg_t null_id;
	int retval = async_req_0_1(exch, DEVMAP_NULL_CREATE, &null_id);
	
	devmap_exchange_end(exch);
	
	if (retval != EOK)
		return -1;
	
	return (int) null_id;
}

void devmap_null_destroy(int null_id)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	async_req_1_0(exch, DEVMAP_NULL_DESTROY, (sysarg_t) null_id);
	devmap_exchange_end(exch);
}

static size_t devmap_count_namespaces_internal(async_exch_t *exch)
{
	sysarg_t count;
	int retval = async_req_0_1(exch, DEVMAP_GET_NAMESPACE_COUNT, &count);
	if (retval != EOK)
		return 0;
	
	return count;
}

static size_t devmap_count_devices_internal(async_exch_t *exch,
    devmap_handle_t ns_handle)
{
	sysarg_t count;
	int retval = async_req_1_1(exch, DEVMAP_GET_DEVICE_COUNT, ns_handle,
	    &count);
	if (retval != EOK)
		return 0;
	
	return count;
}

size_t devmap_count_namespaces(void)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	size_t size = devmap_count_namespaces_internal(exch);
	devmap_exchange_end(exch);
	
	return size;
}

size_t devmap_count_devices(devmap_handle_t ns_handle)
{
	async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
	size_t size = devmap_count_devices_internal(exch, ns_handle);
	devmap_exchange_end(exch);
	
	return size;
}

size_t devmap_get_namespaces(dev_desc_t **data)
{
	/* Loop until namespaces read succesful */
	while (true) {
		async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
		size_t count = devmap_count_namespaces_internal(exch);
		devmap_exchange_end(exch);
		
		if (count == 0)
			return 0;
		
		dev_desc_t *devs = (dev_desc_t *) calloc(count, sizeof(dev_desc_t));
		if (devs == NULL)
			return 0;
		
		exch = devmap_exchange_begin(DEVMAP_CLIENT);
		
		ipc_call_t answer;
		aid_t req = async_send_0(exch, DEVMAP_GET_NAMESPACES, &answer);
		int rc = async_data_read_start(exch, devs, count * sizeof(dev_desc_t));
		
		devmap_exchange_end(exch);
		
		if (rc == EOVERFLOW) {
			/*
			 * Number of namespaces has changed since
			 * the last call of DEVMAP_DEVICE_GET_NAMESPACE_COUNT
			 */
			free(devs);
			continue;
		}
		
		if (rc != EOK) {
			async_wait_for(req, NULL);
			free(devs);
			return 0;
		}
		
		sysarg_t retval;
		async_wait_for(req, &retval);
		
		if (retval != EOK)
			return 0;
		
		*data = devs;
		return count;
	}
}

size_t devmap_get_devices(devmap_handle_t ns_handle, dev_desc_t **data)
{
	/* Loop until devices read succesful */
	while (true) {
		async_exch_t *exch = devmap_exchange_begin_blocking(DEVMAP_CLIENT);
		size_t count = devmap_count_devices_internal(exch, ns_handle);
		devmap_exchange_end(exch);
		
		if (count == 0)
			return 0;
		
		dev_desc_t *devs = (dev_desc_t *) calloc(count, sizeof(dev_desc_t));
		if (devs == NULL)
			return 0;
		
		exch = devmap_exchange_begin(DEVMAP_CLIENT);
		
		ipc_call_t answer;
		aid_t req = async_send_1(exch, DEVMAP_GET_DEVICES, ns_handle, &answer);
		int rc = async_data_read_start(exch, devs, count * sizeof(dev_desc_t));
		
		devmap_exchange_end(exch);
		
		if (rc == EOVERFLOW) {
			/*
			 * Number of devices has changed since
			 * the last call of DEVMAP_DEVICE_GET_DEVICE_COUNT
			 */
			free(devs);
			continue;
		}
		
		if (rc != EOK) {
			async_wait_for(req, NULL);
			free(devs);
			return 0;
		}
		
		sysarg_t retval;
		async_wait_for(req, &retval);
		
		if (retval != EOK)
			return 0;
		
		*data = devs;
		return count;
	}
}
