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
#include <devmap.h>
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

	device_phone = devmap_device_connect(handle, 0);
	if (device_phone < 0) {
		printf("Failed to connect to device (handle = %u).\n",
		    handle);
		return -1;
	}

	printf("Connected to device.\n");
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

/** Test DevMap from the driver's point of view.
 *
 *
 */
char * test_devmap1(bool quiet)
{
	const char *retval = NULL;
	
	/* Register new driver */
	int rc = devmap_driver_register("TestDriver", driver_client_connection);
	if (rc < 0) {
		retval = "Error: Cannot register driver.\n";
		goto out;
	}
	
	/* Register new device dev1. */
	dev_handle_t dev1_handle;
	rc = devmap_device_register(TEST_DEVICE1, &dev1_handle);
	if (rc != EOK) {
		retval = "Error: cannot register device.\n";
		goto out;
	}
	
	/*
	 * Get handle for dev2 (Should fail unless device is already registered
	 * by someone else).
	 */
	dev_handle_t handle;
	rc = devmap_device_get_handle(TEST_DEVICE2, &handle, 0);
	if (rc == EOK) {
		retval = "Error: got handle for dev2 before it was registered.\n";
		goto out;
	}
	
	/* Register new device dev2. */
	dev_handle_t dev2_handle;
	rc = devmap_device_register(TEST_DEVICE2, &dev2_handle);
	if (rc != EOK) {
		retval = "Error: cannot register device dev2.\n";
		goto out;
	}
	
	/* Register device dev1 again. */
	dev_handle_t dev3_handle;
	rc = devmap_device_register(TEST_DEVICE1, &dev3_handle);
	if (rc == EOK) {
		retval = "Error: dev1 registered twice.\n";
		goto out;
	}
	
	/* Get handle for dev1. */
	rc = devmap_device_get_handle(TEST_DEVICE1, &handle, 0);
	if (rc != EOK) {
		retval = "Error: cannot get handle for 'DEVMAP_DEVICE1'.\n";
		goto out;
	}
	
	if (handle != dev1_handle) {
		retval = "Error: cannot get handle for 'DEVMAP_DEVICE1'.\n";
		goto out;
	}
	
	if (device_client(dev1_handle) != EOK) {
		retval = "Error: failed client test for 'DEVMAP_DEVICE1'.\n";
		goto out;
	}
	
out:
	devmap_hangup_phone(DEVMAP_DRIVER);
	devmap_hangup_phone(DEVMAP_CLIENT);
	
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

