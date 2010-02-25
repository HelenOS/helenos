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

#include <string.h>
#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/devman.h>
#include <devman.h>
#include <async.h>
#include <errno.h>
#include <malloc.h>
#include <bool.h>

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
