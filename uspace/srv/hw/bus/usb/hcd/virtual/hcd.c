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
#include "conn.h"


static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipcarg_t phone_hash = icall->in_phone_hash;
	
	ipc_answer_0(iid, EOK);
	printf("%s: new client connected (phone %#x).\n", NAME, phone_hash);
	
	while (true) {
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
			int kind = IPC_GET_ARG1(call);
			int callback = IPC_GET_ARG5(call);
			
			/*
			 * Determine whether host connected to us
			 * or a device.
			 */
			if (kind == 0) {
				ipc_answer_0(callid, EOK);
				connection_handler_host(phone_hash, callback);
				return;
			} else if (kind == 1) {
				int device_id = IPC_GET_ARG2(call);
				virtdev_connection_t *dev
				    = virtdev_recognise(device_id, callback);
				if (!dev) {
					ipc_answer_0(callid, EEXISTS);
					ipc_hangup(callback);
					return;
				}
				ipc_answer_0(callid, EOK);
				connection_handler_device(phone_hash, dev);
				virtdev_destroy_device(dev);
				return;
			} else {
				ipc_answer_0(callid, EINVAL);
				ipc_hangup(callback);
				return;
			}
		}
		
		/*
		 * No other methods could be served now.
		 */
		ipc_answer_0(callid, ENOTSUP);
	}
}

int main(int argc, char * argv[])
{
	printf("%s: Virtual USB host controller driver.\n", NAME);
	
	int rc;
	
	rc = devmap_driver_register(NAME, client_connection); 
	if (rc != EOK) {
		printf("%s: unable to register driver (%s).\n",
		    NAME, str_error(rc));
		return 1;
	}

	rc = devmap_device_register(DEVMAP_PATH, NULL);
	if (rc != EOK) {
		printf("%s: unable to register device %s (%s).\n",
		    NAME, DEVMAP_PATH, str_error(rc));
		return 1;
	}
	
	printf("%s: accepting connections.\n", NAME);
	hc_manager();
	
	return 0;
}


/**
 * @}
 */
