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
/**
 * @file
 * @brief Simple application that connects to USB/HCD.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vfs/vfs.h>
#include <fcntl.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <async.h>

#include <usb/hcd.h>

#define LOOPS 5
#define MAX_SIZE_RECEIVE 64
#define NAME "hcd-example"

#define DEV_HCD_NAME "hcd-virt"

#define __QUOTEME(x) #x
#define _QUOTEME(x) __QUOTEME(x)

#define VERBOSE_EXEC(cmd, fmt, ...) \
	(printf("%s: %s" fmt "\n", NAME, _QUOTEME(cmd), __VA_ARGS__), cmd(__VA_ARGS__))

static void fibril_sleep(size_t sec)
{
	while (sec-- > 0) {
		async_usleep(1000*1000);
	}
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_answer_0(iid, EOK);
	printf("%s: client connection()\n", NAME);
	
	while (true) {
		ipc_callid_t callid; 
		ipc_call_t call; 
		int rc;
		void * buffer;
		size_t len;
		
		callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
			case IPC_M_USB_HCD_DATA_SENT:
				printf("%s: >> Data sent over USB (handle %d, outcome %s).\n",
				    NAME, IPC_GET_ARG1(call),
				    usb_str_transaction_outcome(IPC_GET_ARG2(call)));
				ipc_answer_0(callid, EOK);
				break;
			case IPC_M_USB_HCD_DATA_RECEIVED:
				printf("%s: << Data received over USB (handle %d, outcome %s).\n",
				    NAME, IPC_GET_ARG1(call),
				    usb_str_transaction_outcome(IPC_GET_ARG2(call)));
				if (IPC_GET_ARG2(call) != USB_OUTCOME_OK) {
					ipc_answer_0(callid, EOK);
					break;
				}
				rc = async_data_write_accept(&buffer, false,
				    1, MAX_SIZE_RECEIVE,
				    0, &len);
				if (rc != EOK) {
					ipc_answer_0(callid, rc);
					break;
				}
				printf("%s: << Received %uB long buffer (handle %d).\n",
				    NAME, len, IPC_GET_ARG1(call));
				ipc_answer_0(callid, EOK);
				break;
			default:
				ipc_answer_0(callid, EINVAL);
				break;
		}
	}
}

int main(int argc, char * argv[])
{
	int hcd_phone = usb_hcd_create_phones(DEV_HCD_NAME, client_connection);
	if (hcd_phone < 0) {
		printf("%s: Unable to start comunication with HCD at usb://%s (%d: %s).\n",
		    NAME, DEV_HCD_NAME, hcd_phone, str_error(hcd_phone));
		return 1;
	}
	
	char data[] = "Hullo, World!";
	int data_len = sizeof(data)/sizeof(data[0]);
	
	size_t i;
	for (i = 0; i < LOOPS; i++) {
		usb_transaction_handle_t handle;

		usb_target_t target = { 11 + i, 3 };
		int rc = usb_hcd_send_data_to_function(hcd_phone,
		    target, USB_TRANSFER_ISOCHRONOUS,
		    data, data_len,
		    &handle);
		if (rc != EOK) {
			printf("%s: >> Failed to send data to function over HCD (%d: %s).\n",
				NAME, rc, str_error(rc));
			continue;
		}

		printf("%s: >> Transaction to function dispatched (handle %d).\n", NAME, handle);
		
		fibril_sleep(1);
		
		rc = usb_hcd_prepare_data_reception(hcd_phone,
		    target, USB_TRANSFER_INTERRUPT,
		    MAX_SIZE_RECEIVE,
		    &handle);
		if (rc != EOK) {
			printf("%s: << Failed to start transaction for data receivement over HCD (%d: %s).\n",
				NAME, rc, str_error(rc));
			continue;
		}
		
		printf("%s: << Transaction from function started (handle %d).\n", NAME, handle);
		
		fibril_sleep(2);
	}
	
	printf("%s: Waiting for transactions to be finished...\n", NAME);
	fibril_sleep(10);
	
	printf("%s: exiting.\n", NAME);
	
	ipc_hangup(hcd_phone);
	
	return 0;
}


/** @}
 */
