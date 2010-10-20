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

#include <str.h>
#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/devman.h>
#include <devman.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>
#include <adt/list.h>

static int devman_phone_driver = -1;
static int devman_phone_client = -1;

int devman_get_phone(devman_interface_t iface, unsigned int flags)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		if (devman_phone_driver >= 0)
			return devman_phone_driver;
		
		if (flags & IPC_FLAG_BLOCKING)
			devman_phone_driver = ipc_connect_me_to_blocking(PHONE_NS,
			    SERVICE_DEVMAN, DEVMAN_DRIVER, 0);
		else
			devman_phone_driver = ipc_connect_me_to(PHONE_NS,
			    SERVICE_DEVMAN, DEVMAN_DRIVER, 0);
		
		return devman_phone_driver;
	case DEVMAN_CLIENT:
		if (devman_phone_client >= 0)
			return devman_phone_client;
		
		if (flags & IPC_FLAG_BLOCKING)
			devman_phone_client = ipc_connect_me_to_blocking(PHONE_NS,
			    SERVICE_DEVMAN, DEVMAN_CLIENT, 0);
		else
			devman_phone_client = ipc_connect_me_to(PHONE_NS,
			    SERVICE_DEVMAN, DEVMAN_CLIENT, 0);
		
		return devman_phone_client;
	default:
		return -1;
	}
}

/** Register running driver with device manager. */
int devman_driver_register(const char *name, async_client_conn_t conn)
{
	int phone = devman_get_phone(DEVMAN_DRIVER, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAN_DRIVER_REGISTER, 0, 0, &answer);
	
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

static int devman_send_match_id(int phone, match_id_t *match_id) \
{
	ipc_call_t answer;
	async_send_1(phone, DEVMAN_ADD_MATCH_ID, match_id->score, &answer);
	int retval = async_data_write_start(phone, match_id->id, str_size(match_id->id));
	return retval;	
}


static int devman_send_match_ids(int phone, match_id_list_t *match_ids) 
{
	link_t *link = match_ids->ids.next;
	match_id_t *match_id = NULL;
	int ret = EOK;
	
	while (link != &match_ids->ids) {
		match_id = list_get_instance(link, match_id_t, link); 
		if (EOK != (ret = devman_send_match_id(phone, match_id))) 
		{
			printf("Driver failed to send match id, error number = %d\n", ret);
			return ret;			
		}
		link = link->next;
	}
	return ret;	
}

int devman_child_device_register(
	const char *name, match_id_list_t *match_ids, device_handle_t parent_handle, device_handle_t *handle)
{		
	int phone = devman_get_phone(DEVMAN_DRIVER, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	int match_count = list_count(&match_ids->ids);	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAN_ADD_CHILD_DEVICE, parent_handle, match_count, &answer);

	ipcarg_t retval = async_data_write_start(phone, name, str_size(name));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return retval;
	}
	
	devman_send_match_ids(phone, match_ids);
	
	async_wait_for(req, &retval);
	
	async_serialize_end();
	
	if (retval != EOK) {
		if (handle != NULL) {
			*handle = -1;
		}
		return retval;
	}	
	
	if (handle != NULL)
		*handle = (int) IPC_GET_ARG1(answer);	
		
	return retval;
}

int devman_add_device_to_class(device_handle_t dev_handle, const char *class_name)
{
	int phone = devman_get_phone(DEVMAN_DRIVER, IPC_FLAG_BLOCKING);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	ipc_call_t answer;
	aid_t req = async_send_1(phone, DEVMAN_ADD_DEVICE_TO_CLASS, dev_handle, &answer);
	
	ipcarg_t retval = async_data_write_start(phone, class_name, str_size(class_name));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return retval;
	}
	
	async_wait_for(req, &retval);
	async_serialize_end();
	
	return retval;	
}

void devman_hangup_phone(devman_interface_t iface)
{
	switch (iface) {
	case DEVMAN_DRIVER:
		if (devman_phone_driver >= 0) {
			ipc_hangup(devman_phone_driver);
			devman_phone_driver = -1;
		}
		break;
	case DEVMAN_CLIENT:
		if (devman_phone_client >= 0) {
			ipc_hangup(devman_phone_client);
			devman_phone_client = -1;
		}
		break;
	default:
		break;
	}
}

int devman_device_connect(device_handle_t handle, unsigned int flags)
{
	int phone;
	
	if (flags & IPC_FLAG_BLOCKING) {
		phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_DEVICE, handle);
	} else {
		phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_DEVICE, handle);
	}
	
	return phone;
}

int devman_parent_device_connect(device_handle_t handle, unsigned int flags)
{
	int phone;
	
	if (flags & IPC_FLAG_BLOCKING) {
		phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	} else {
		phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAN,
		    DEVMAN_CONNECT_TO_PARENTS_DEVICE, handle);
	}
	
	return phone;
}

int devman_device_get_handle(const char *pathname, device_handle_t *handle, unsigned int flags)
{
	int phone = devman_get_phone(DEVMAN_CLIENT, flags);
	
	if (phone < 0)
		return phone;
	
	async_serialize_start();
	
	ipc_call_t answer;
	aid_t req = async_send_2(phone, DEVMAN_DEVICE_GET_HANDLE, flags, 0,
	    &answer);
	
	ipcarg_t retval = async_data_write_start(phone, pathname, str_size(pathname));
	if (retval != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		return retval;
	}
	
	async_wait_for(req, &retval);
	
	async_serialize_end();
	
	if (retval != EOK) {
		if (handle != NULL)
			*handle = (device_handle_t) -1;
		return retval;
	}
	
	if (handle != NULL)
		*handle = (device_handle_t) IPC_GET_ARG1(answer);
	
	return retval;
}


/** @}
 */