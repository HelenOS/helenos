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
#include <../../../srv/devmap/devmap.h>
#include "../tester.h"

static int driver_register(char *name)
{
	int retval;
	aid_t req;
	ipc_call_t answer;
	int phone;
	ipcarg_t callback_phonehash;

	phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP, DEVMAP_DRIVER);

	while (phone < 0) {
		usleep(100000);
		phone = ipc_connect_me_to(PHONE_NS, SERVICE_DEVMAP, DEVMAP_DRIVER);
	}
	
	req = async_send_2(phone, DEVMAP_DRIVER_REGISTER, 0, 0, &answer);

	retval = ipc_data_send(phone, (char *)name, strlen(name)); 

	if (retval != EOK) {
		async_wait_for(req, NULL);
		return -1;
	}

	ipc_connect_to_me(phone, 0, 0, &callback_phonehash);

	async_wait_for(req, NULL);
	printf("Driver '%s' registered.\n", name);

	return phone;
}

static int device_register(int driver_phone, char *name, int *handle)
{
	ipcarg_t retval;
	aid_t req;
	ipc_call_t answer;

	req = async_send_2(driver_phone, DEVMAP_DEVICE_REGISTER, 0, 0, &answer);

	retval = ipc_data_send(driver_phone, (char *)name, strlen(name)); 

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
		printf("Device registered with handle %u.\n", (int) IPC_GET_ARG1(answer));
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

	/* Register new driver */
	driver_phone = driver_register("TestDriver");

	if (driver_phone < 0) {
		return "Error: Cannot register driver.\n";	
	}

	/* Register new device dev1*/
	if (EOK != device_register(driver_phone, "TestDevice1", &dev1_handle)) {
		ipc_hangup(driver_phone);
		return "Error: cannot register device.\n";
	}
	
	/* TODO: get handle for dev1*/

	/* TODO: get handle for dev2 (Should fail unless device is already 
	 * registered by someone else) 
	 */

	/* Register new device dev2*/
	if (EOK != device_register(driver_phone, "TestDevice2", &dev2_handle)) {
		ipc_hangup(driver_phone);
		return "Error: cannot register device dev2.\n";
	}


	/* Register again device dev1 */
	if (EOK == device_register(driver_phone, "TestDevice1", &dev3_handle)) {
		return "Error: dev1 registered twice.\n";
	}

	/* TODO: get handle for dev2 */

	/* TODO:  */

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

