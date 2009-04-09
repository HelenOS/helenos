/*
 * Copyright (c) 2007 Josef Cejka
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

#include <stdio.h>
#include <unistd.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <errno.h>
#include <ipc/devmap.h>
#include "../tester.h"

#include <time.h>

#define TEST_DEVICE1 "TestDevice1"
#define TEST_DEVICE2 "TestDevice2"

/** Handle requests from clients 
 *
 */
static void driver_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	
	printf("connected: method=%u arg1=%u, arg2=%u arg3=%u.\n",
	    IPC_GET_METHOD(*icall), IPC_GET_ARG1(*icall), IPC_GET_ARG2(*icall),
	    IPC_GET_ARG3(*icall));

	printf("driver_client_connection.\n");
	ipc_answer_0(iid, EOK);

	/* Ignore parameters, the connection is already opened */
	while (1) {
		callid = async_get_call(&call);
		retval = EOK;
		printf("method=%u arg1=%u, arg2=%u arg3=%u.\n",
		    IPC_GET_METHOD(call), IPC_GET_ARG1(call),
		    IPC_GET_ARG2(call), IPC_GET_ARG3(call));
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/* TODO: Handle hangup */
			return;
		default:
			printf("Unknown device method %u.\n",
			    IPC_GET_METHOD(call));
			retval = ENOENT;
		}
		ipc_answer_0(callid, retval);
	}
	return;
}

static int device_client_fibril(void *arg)
{
	int handle;
	int device_phone;

	handle = (int)arg;

	device_phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP,
	    DEVMAP_CONNECT_TO_DEVICE, handle);

	if (device_phone < 0) {
		printf("Failed to connect to devmap as client (handle = %u).\n",
		    handle);
		return -1;
	}
/*	
 *	device_phone = (int) IPC_GET_ARG5(answer);
 */
	printf("Connected to device.\n");
	ipc_call_sync_1_0(device_phone, 1024, 1025);
/*
 * ipc_hangup(device_phone);
 */
	ipc_hangup(device_phone);

	return EOK;
}

/** Communication test with device.
 * @param handle handle to tested instance.
 */
static int device_client(int handle)
{
/*	fid_t fid;
	ipc_call_t call;
	ipc_callid_t callid;

	fid = fibril_create(device_client_fibril, (void *)handle);
	fibril_add_ready(fid);

*/
	return EOK;
}

/**
 *
 */
static int driver_register(char *name)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;
	int phone;
	ipcarg_t callback_phonehash;

	phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_DEVMAP, DEVMAP_DRIVER, 0);
	if (phone < 0) {
		printf("Failed to connect to device mapper\n");
		return -1;
	}
	
	req = async_send_2(phone, DEVMAP_DRIVER_REGISTER, 0, 0, &answer);

	retval = ipc_data_write_start(phone, (char *)name, str_size(name) + 1); 

	if (retval != EOK) {
		async_wait_for(req, NULL);
		return -1;
	}

	async_set_client_connection(driver_client_connection);

	ipc_connect_to_me(phone, 0, 0, 0, &callback_phonehash);
/*	
	if (NULL == async_new_connection(callback_phonehash, 0, NULL,
			driver_client_connection)) {
		printf("Failed to create new fibril.\n");	
		async_wait_for(req, NULL);
		return -1;
	}
*/
	async_wait_for(req, &retval);
	printf("Driver '%s' registered.\n", name);

	return phone;
}

static int device_get_handle(int driver_phone, char *name, int *handle)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;

	req = async_send_2(driver_phone, DEVMAP_DEVICE_GET_HANDLE, 0, 0,
	    &answer);

	retval = ipc_data_write_start(driver_phone, name, str_size(name) + 1);

	if (retval != EOK) {
		printf("Failed to send device name '%s'.\n", name);
		async_wait_for(req, NULL);
		return retval;
	}

	async_wait_for(req, &retval);

	if (NULL != handle) {
		*handle = -1;
	}

	if (EOK == retval) {
		
		if (NULL != handle) {
			*handle = (int) IPC_GET_ARG1(answer);
		}
		printf("Device '%s' has handle %u.\n", name,
		    (int) IPC_GET_ARG1(answer));
	} else {
		printf("Failed to get handle for device '%s'.\n", name);
	}

	return retval;
}

/** Register new device.
 * @param driver_phone
 * @param name Device name.
 * @param handle Output variable. Handle to the created instance of device.
 */
static int device_register(int driver_phone, char *name, int *handle)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;

	req = async_send_2(driver_phone, DEVMAP_DEVICE_REGISTER, 0, 0, &answer);

	retval = ipc_data_write_start(driver_phone, (char *)name,
	    str_size(name) + 1);

	if (retval != EOK) {
		printf("Failed to send device name '%s'.\n", name);
		async_wait_for(req, NULL);
		return retval;
	}

	async_wait_for(req, &retval);

	if (NULL != handle) {
		*handle = -1;
	}

	if (EOK == retval) {
		
		if (NULL != handle) {
			*handle = (int) IPC_GET_ARG1(answer);
		}
		printf("Device registered with handle %u.\n",
		    (int) IPC_GET_ARG1(answer));
	}

	return retval;
}

/** Test DevMap from the driver's point of view.
 *
 *
 */
char * test_devmap1(bool quiet)
{
	int driver_phone;
	int dev1_handle;
	int dev2_handle;
	int dev3_handle;
	int handle;

	/* Register new driver */
	driver_phone = driver_register("TestDriver");

	if (driver_phone < 0) {
		return "Error: Cannot register driver.\n";	
	}

	/* Register new device dev1*/
	if (EOK != device_register(driver_phone, TEST_DEVICE1, &dev1_handle)) {
		ipc_hangup(driver_phone);
		return "Error: cannot register device.\n";
	}

	/* Get handle for dev2 (Should fail unless device is already 
	 * registered by someone else) 
	 */
	if (EOK == device_get_handle(driver_phone, TEST_DEVICE2, &handle)) {
		ipc_hangup(driver_phone);
		return "Error: got handle for dev2 before it was registered.\n";
	}

	/* Register new device dev2*/
	if (EOK != device_register(driver_phone, TEST_DEVICE2, &dev2_handle)) {
		ipc_hangup(driver_phone);
		return "Error: cannot register device dev2.\n";
	}

	/* Register again device dev1 */
	if (EOK == device_register(driver_phone, TEST_DEVICE1, &dev3_handle)) {
		return "Error: dev1 registered twice.\n";
	}

	/* Get handle for dev1*/
	if (EOK != device_get_handle(driver_phone, TEST_DEVICE1, &handle)) {
		ipc_hangup(driver_phone);
		return "Error: cannot get handle for 'DEVMAP_DEVICE1'.\n";
	}

	if (handle != dev1_handle) {
		ipc_hangup(driver_phone);
		return "Error: cannot get handle for 'DEVMAP_DEVICE1'.\n";
	}

	if (EOK != device_client(dev1_handle)) {
		ipc_hangup(driver_phone);
		return "Error: failed client test for 'DEVMAP_DEVICE1'.\n";
	}

	/* TODO: */

	ipc_hangup(driver_phone);

	return NULL;
}

char *test_devmap2(bool quiet)
{
	/*TODO: Full automatic test */
	return NULL;
}

char *test_devmap3(bool quiet)
{
	/* TODO: allow user to call test functions in random order */
	return NULL;
}

