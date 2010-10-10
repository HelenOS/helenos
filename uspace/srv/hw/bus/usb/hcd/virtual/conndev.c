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
 * @brief Connection handling of calls from virtual device (implementation).
 */

#include <assert.h>
#include <errno.h>
#include <usb/virtdev.h>

#include "conn.h"
#include "hc.h"

/** Handle data from device to function.
 */
static void handle_data_from_device(ipc_callid_t iid, ipc_call_t icall,
    virtdev_connection_t *dev)
{
	usb_endpoint_t endpoint = IPC_GET_ARG1(icall);
	usb_target_t target = {
		.address = dev->address,
		.endpoint = endpoint
	};
	
	dprintf("data from device %d [%d.%d]", dev->id,
	    target.address, target.endpoint);
	
	size_t len;
	void * buffer;
	int rc = async_data_write_accept(&buffer, false,
	    1, USB_MAX_PAYLOAD_SIZE,
	    0, &len);
	
	if (rc != EOK) {
		ipc_answer_0(iid, rc);
		return;
	}
	
	rc = hc_fillin_transaction_from_device(USB_TRANSFER_INTERRUPT, target, buffer, len);
	
	ipc_answer_0(iid, rc);
}

/** Connection handler for communcation with virtual device.
 *
 * @param phone_hash Incoming phone hash.
 * @param dev Virtual device handle.
 */
void connection_handler_device(ipcarg_t phone_hash, virtdev_connection_t *dev)
{
	assert(dev != NULL);
	
	dprintf("phone%#x: virtual device %d connected [%d]",
	    phone_hash, dev->id, dev->address);
	
	while (true) {
		ipc_callid_t callid; 
		ipc_call_t call; 
		
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				ipc_hangup(dev->phone);
				ipc_answer_0(callid, EOK);
				dprintf("phone%#x: device %d [%d] hang-up",
				    phone_hash, dev->id, dev->address);
				return;
			
			case IPC_M_CONNECT_TO_ME:
				ipc_answer_0(callid, ELIMIT);
				break;
			
			case IPC_M_USB_VIRTDEV_DATA_FROM_DEVICE:
				handle_data_from_device(callid, call, dev);
				break;
			
			default:
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

/**
 * @}
 */
