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
#include <usbvirt/hub.h>

#include "conn.h"
#include "hc.h"
#include "hub.h"

/** Connection handler for communcation with virtual device.
 *
 * This function also takes care of proper phone hung-up.
 *
 * @param phone_hash Incoming phone hash.
 * @param dev Virtual device handle.
 */
void connection_handler_device(ipcarg_t phone_hash, virtdev_connection_t *dev)
{
	assert(dev != NULL);
	
	dprintf(0, "virtual device connected through phone %#x", phone_hash);
	
	while (true) {
		ipc_callid_t callid; 
		ipc_call_t call; 
		
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_PHONE_HUNGUP:
				ipc_hangup(dev->phone);
				ipc_answer_0(callid, EOK);
				dprintf(0, "phone%#x: device hung-up",
				    phone_hash);
				return;
			
			case IPC_M_CONNECT_TO_ME:
				ipc_answer_0(callid, ELIMIT);
				break;
			
			default:
				dprintf_inval_call(2, call, phone_hash);
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

/**
 * @}
 */
