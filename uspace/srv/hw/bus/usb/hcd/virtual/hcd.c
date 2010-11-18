/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */ 
/** @file
 * @brief Virtual host controller driver.
 */

#include <devmap.h>
#include <ipc/ipc.h>
#include <async.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>

#include <usb/hcd.h>
#include "vhcd.h"
#include "hc.h"
#include "devices.h"
#include "hub.h"
#include "conn.h"


static devmap_handle_t handle_virtual_device;
static devmap_handle_t handle_host_driver;

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipcarg_t phone_hash = icall->in_phone_hash;
	devmap_handle_t handle = (devmap_handle_t)IPC_GET_ARG1(*icall);
	
	if (handle == handle_host_driver) {
		/*
		 * We can connect host controller driver immediately.
		 */
		ipc_answer_0(iid, EOK);
		connection_handler_host(phone_hash);
	} else if (handle == handle_virtual_device) {
		ipc_answer_0(iid, EOK);

		while (true) {
			/*
			 * We need to wait for callback request to allow
			 * connection of virtual device.
			 */
			ipc_callid_t callid;
			ipc_call_t call;

			callid = async_get_call(&call);

			/*
			 * We can do nothing until we have the callback phone.
			 * Thus, we will wait for the callback and start processing
			 * after that.
			 */
			int method = (int) IPC_GET_METHOD(call);

			if (method == IPC_M_PHONE_HUNGUP) {
				ipc_answer_0(callid, EOK);
				return;
			}

			if (method == IPC_M_CONNECT_TO_ME) {
				int callback = IPC_GET_ARG5(call);
				virtdev_connection_t *dev
				    = virtdev_add_device(callback);
				if (!dev) {
					ipc_answer_0(callid, EEXISTS);
					ipc_hangup(callback);
					return;
				}
				ipc_answer_0(callid, EOK);
				connection_handler_device(phone_hash, dev);
				virtdev_destroy_device(dev);
				return;
			}

			/*
			 * No other methods could be served now.
			 */
			dprintf_inval_call(1, call, phone_hash);
			ipc_answer_0(callid, ENOTSUP);
		}
	} else {
		/*
		 * Hmmm, someone else just tried to connect to us.
		 * Kick him out ;-).
		 */
		ipc_answer_0(iid, ENOTSUP);
		return;
	}
}

int main(int argc, char * argv[])
{	
	printf("%s: virtual USB host controller driver.\n", NAME);
	
	int i;
	for (i = 1; i < argc; i++) {
		if (str_cmp(argv[i], "-d") == 0) {
			debug_level++;
		}
	}
	
	int rc;
	
	rc = devmap_driver_register(NAME, client_connection); 
	if (rc != EOK) {
		printf("%s: unable to register driver (%s).\n",
		    NAME, str_error(rc));
		return 1;
	}

	rc = devmap_device_register(DEVMAP_PATH_HC, &handle_host_driver);
	if (rc != EOK) {
		printf("%s: unable to register device %s (%s).\n",
		    NAME, DEVMAP_PATH_HC, str_error(rc));
		return 1;
	}
	
	rc = devmap_device_register(DEVMAP_PATH_DEV, &handle_virtual_device);
	if (rc != EOK) {
		printf("%s: unable to register device %s (%s).\n",
		    NAME, DEVMAP_PATH_DEV, str_error(rc));
		return 1;
	}

	hub_init();
	
	printf("%s: accepting connections [debug=%d]\n", NAME, debug_level);
	printf("%s:  -> host controller at %s\n", NAME, DEVMAP_PATH_HC);
	printf("%s:  -> virtual hub at %s\n", NAME, DEVMAP_PATH_DEV);

	hc_manager();
	
	return 0;
}


/**
 * @}
 */
