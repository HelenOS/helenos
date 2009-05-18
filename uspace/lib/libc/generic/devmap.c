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

#include <string.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/devmap.h>
#include <devmap.h>
#include <async.h>
#include <errno.h>

static int devmap_phone = -1;

/** Get phone to device mapper task. */
static int devmap_get_phone(unsigned int flags)
{
	int phone;

	if (devmap_phone >= 0)
		return devmap_phone;

	if (flags & IPC_FLAG_BLOCKING) {
		phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_DEVMAP,
		    DEVMAP_CLIENT, 0);
	} else {
		phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
		    DEVMAP_CLIENT, 0);
	}

	if (phone < 0)
		return phone;

	devmap_phone = phone;
	return phone;
}

/** Register new driver with devmap. */
int devmap_driver_register(const char *name, async_client_conn_t conn)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;
	int phone;
	ipcarg_t callback_phonehash;

	phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_DEVMAP,
	    DEVMAP_DRIVER, 0);

	if (phone < 0) {
		return phone;
	}
	
	req = async_send_2(phone, DEVMAP_DRIVER_REGISTER, 0, 0, &answer);
	retval = ipc_data_write_start(phone, name, str_size(name) + 1); 

	if (retval != EOK) {
		async_wait_for(req, NULL);
		ipc_hangup(phone);
		return -1;
	}

	async_set_client_connection(conn);

	ipc_connect_to_me(phone, 0, 0, 0, &callback_phonehash);
	async_wait_for(req, &retval);

	return phone;
}

int devmap_device_get_handle(const char *name, dev_handle_t *handle,
    unsigned int flags)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;
	int phone;

	phone = devmap_get_phone(flags);
	if (phone < 0)
		return phone;

	req = async_send_2(phone, DEVMAP_DEVICE_GET_HANDLE, flags, 0,
	    &answer);

	retval = ipc_data_write_start(phone, name, str_size(name) + 1);

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
		*handle = (int) IPC_GET_ARG1(answer);

	return retval;
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

/** Register new device.
 *
 * @param driver_phone
 * @param name Device name.
 * @param handle Output: Handle to the created instance of device.
 */
int devmap_device_register(int driver_phone, const char *name, int *handle)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;

	req = async_send_2(driver_phone, DEVMAP_DEVICE_REGISTER, 0, 0,
	    &answer);

	retval = ipc_data_write_start(driver_phone, name, str_size(name) + 1);

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

	*handle = (int) IPC_GET_ARG1(answer);
	return retval;
}
