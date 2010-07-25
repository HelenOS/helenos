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
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/devmap.h>
#include <devmap.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>

static int devmap_phone_driver = -1;
static int devmap_phone_client = -1;

/** Get phone to device mapper task. */
int devmap_get_phone(devmap_interface_t iface, unsigned int flags)
{
	switch (iface) {
	case DEVMAP_DRIVER:
		if (devmap_phone_driver >= 0)
			return devmap_phone_driver;
		
		if (flags & IPC_FLAG_BLOCKING)
			devmap_phone_driver = ipc_connect_me_to_blocking(PHONE_NS,
			    SERVICE_DEVMAP, DEVMAP_DRIVER, 0);
		else
			devmap_phone_driver = ipc_connect_me_to(PHONE_NS,
			    SERVICE_DEVMAP, DEVMAP_DRIVER, 0);
		
		return devmap_phone_driver;
	case DEVMAP_CLIENT:
		if (devmap_phone_client >= 0)
			return devmap_phone_client;
		
		if (flags & IPC_FLAG_BLOCKING)
			devmap_phone_client = ipc_connect_me_to_blocking(PHONE_NS,
			    SERVICE_DEVMAP, DEVMAP_CLIENT, 0);
		else
			devmap_phone_client = ipc_connect_me_to(PHONE_NS,
			    SERVICE_DEVMAP, DEVMAP_CLIENT, 0);
		
		return devmap_phone_client;
	default:
		return -1;
	}
}

void devmap_hangup_phone(devmap_interface_t iface)
{
	switch (iface) {
	case DEVMAP_DRIVER:
		if (devmap_phone_driver >= 0) {
			ipc_hangup(devmap_phone_driver);
			devmap_phone_driver = -1;
		}
		break;
	case DEVMAP_CLIENT:
		if (devmap_phone_client >= 0) {
			ipc_hangup(devmap_phone_client);
			devmap_phone_client = -1;
		}
		break;
	default:
		break;
	}
}

/** Register new driver with devmap. */
int devmap_driver_register(const char *name, async_client_conn_t conn)
{
	int phone = devmap_get_phone(DEVMAP_DRIVER, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAP_DRIVER_REGISTER, 0, 0, &answer);
	
	ipcarg_t retval = async_data_write_start(phone, name, str_size(name));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return -1;
	}
	
	async_set_client_connection(conn);
	
	ipcarg_t callback_phonehash;
	ipc_connect_to_me(phone, 0, 0, 0, &callback_phonehash);
	async_wait_for(req, &retval);
	
	async_serialize_end();
	
	return retval;
}

/** Register new device.
 *
 * @param namespace Namespace name.
 * @param fqdn      Fully qualified device name.
 * @param handle    Output: Handle to the created instance of device.
 *
 */
int devmap_device_register(const char *fqdn, dev_handle_t *handle)
{
	int phone = devmap_get_phone(DEVMAP_DRIVER, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAP_DEVICE_REGISTER, 0, 0,
	    &answer);
	
	ipcarg_t retval = async_data_write_start(phone, fqdn, str_size(fqdn));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	async_serialize_end();
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = -1;
		return retval;
	}
	
	if (handle != NULL)
		*handle = (dev_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

int devmap_device_get_handle(const char *fqdn, dev_handle_t *handle, unsigned int flags)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, flags);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAP_DEVICE_GET_HANDLE, flags, 0,
	    &answer);
	
	ipcarg_t retval = async_data_write_start(phone, fqdn, str_size(fqdn));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	async_serialize_end();
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (dev_handle_t) -1;
		return retval;
	}
	
	if (handle != NULL)
		*handle = (dev_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

int devmap_namespace_get_handle(const char *name, dev_handle_t *handle, unsigned int flags)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, flags);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAP_NAMESPACE_GET_HANDLE, flags, 0,
	    &answer);
	
	ipcarg_t retval = async_data_write_start(phone, name, str_size(name));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	async_serialize_end();
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (dev_handle_t) -1;
		return retval;
	}
	
	if (handle != NULL)
		*handle = (dev_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}

devmap_handle_type_t devmap_handle_probe(dev_handle_t handle)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return phone;
	
	ipcarg_t type;
	int retval = async_req_1_1(phone, DEVMAP_HANDLE_PROBE, handle, &type);
	if (retval != EOK)
		return DEV_HANDLE_NONE;
	
	return (devmap_handle_type_t) type;
}

int devmap_device_connect(dev_handle_t handle, unsigned int flags)
{
	int phone;
	
	if (flags & IPC_FLAG_BLOCKING) {
		phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_DEVMAP,
		    DEVMAP_CONNECT_TO_DEVICE, handle);
	} else {
		phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
		    DEVMAP_CONNECT_TO_DEVICE, handle);
	}
	
	return phone;
}

int devmap_null_create(void)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return -1;
	
	ipcarg_t null_id;
	int retval = async_req_0_1(phone, DEVMAP_NULL_CREATE, &null_id);
	if (retval != EOK)
		return -1;
	
	return (int) null_id;
}

void devmap_null_destroy(int null_id)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return;
	
	async_req_1_0(phone, DEVMAP_NULL_DESTROY, (ipcarg_t) null_id);
}

static size_t devmap_count_namespaces_internal(int phone)
{
	ipcarg_t count;
	int retval = async_req_0_1(phone, DEVMAP_GET_NAMESPACE_COUNT, &count);
	if (retval != EOK)
		return 0;
	
	return count;
}

static size_t devmap_count_devices_internal(int phone, dev_handle_t ns_handle)
{
	ipcarg_t count;
	int retval = async_req_1_1(phone, DEVMAP_GET_DEVICE_COUNT, ns_handle, &count);
	if (retval != EOK)
		return 0;
	
	return count;
}

size_t devmap_count_namespaces(void)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return 0;
	
	return devmap_count_namespaces_internal(phone);
}

size_t devmap_count_devices(dev_handle_t ns_handle)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return 0;
	
	return devmap_count_devices_internal(phone, ns_handle);
}

size_t devmap_get_namespaces(dev_desc_t **data)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return 0;
	
	/* Loop until namespaces read succesful */
	while (true) {
		size_t count = devmap_count_namespaces_internal(phone);
		if (count == 0)
			return 0;
		
		dev_desc_t *devs = (dev_desc_t *) calloc(count, sizeof(dev_desc_t));
		if (devs == NULL)
			return 0;
		
		async_serialize_start();
		
		ipc_call_t answer;
		aid_t req = async_send_0(phone, DEVMAP_GET_NAMESPACES, &answer);
		
		int rc = async_data_read_start(phone, devs, count * sizeof(dev_desc_t));
		if (rc == EOVERFLOW) {
			/*
			 * Number of namespaces has changed since
			 * the last call of DEVMAP_DEVICE_GET_NAMESPACE_COUNT
			 */
			async_serialize_end();
			free(devs);
			continue;
		}
		
		if (rc != EOK) {
			async_wait_for(req, NULL);
			async_serialize_end();
			free(devs);
			return 0;
		}
		
		ipcarg_t retval;
		async_wait_for(req, &retval);
		async_serialize_end();
		
		if (retval != EOK)
			return 0;
		
		*data = devs;
		return count;
	}
}

size_t devmap_get_devices(dev_handle_t ns_handle, dev_desc_t **data)
{
	int phone = devmap_get_phone(DEVMAP_CLIENT, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return 0;
	
	/* Loop until namespaces read succesful */
	while (true) {
		size_t count = devmap_count_devices_internal(phone, ns_handle);
		if (count == 0)
			return 0;
		
		dev_desc_t *devs = (dev_desc_t *) calloc(count, sizeof(dev_desc_t));
		if (devs == NULL)
			return 0;
		
		async_serialize_start();
		
		ipc_call_t answer;
		aid_t req = async_send_1(phone, DEVMAP_GET_DEVICES, ns_handle, &answer);
		
		int rc = async_data_read_start(phone, devs, count * sizeof(dev_desc_t));
		if (rc == EOVERFLOW) {
			/*
			 * Number of devices has changed since
			 * the last call of DEVMAP_DEVICE_GET_DEVICE_COUNT
			 */
			async_serialize_end();
			free(devs);
			continue;
		}
		
		if (rc != EOK) {
			async_wait_for(req, NULL);
			async_serialize_end();
			free(devs);
			return 0;
		}
		
		ipcarg_t retval;
		async_wait_for(req, &retval);
		async_serialize_end();
		
		if (retval != EOK)
			return 0;
		
		*data = devs;
		return count;
	}
}
