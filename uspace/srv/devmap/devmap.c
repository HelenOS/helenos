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

/**
 * @defgroup devmap Device mapper.
 * @brief	HelenOS device mapper.
 * @{
 */ 

/** @file
 */

#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <futex.h>
#include <stdlib.h>
#include <string.h>

#include "devmap.h"


LIST_INITIALIZE(devices_list);
LIST_INITIALIZE(drivers_list);

/* order of locking:
 * drivers_list_futex
 * devices_list_futex
 * (devmap_driver_t *)->devices_futex
 * create_handle_futex
 **/

static atomic_t devices_list_futex = FUTEX_INITIALIZER;
static atomic_t drivers_list_futex = FUTEX_INITIALIZER;
static atomic_t create_handle_futex = FUTEX_INITIALIZER;


static int devmap_create_handle(void)
{
	static int last_handle = 0;
	int handle;

	/* TODO: allow reusing old handles after their unregistration
		and implement some version of LRU algorithm */
	/* FIXME: overflow */
	futex_down(&create_handle_futex);	

	last_handle += 1;
	handle = last_handle;

	futex_up(&create_handle_futex);	

	return handle;
}


/** Initialize device mapper. 
 *
 *
 */
static int devmap_init()
{
	/* TODO: */

	return EOK;
}

/** Find device with given name.
 *
 */
static devmap_device_t *devmap_device_find_name(const char *name)
{
	link_t *item;
	devmap_device_t *device = NULL;

	item = devices_list.next;

	while (item != &devices_list) {

		device = list_get_instance(item, devmap_device_t, devices);
		if (0 == strcmp(device->name, name)) {
			break;
		}
		item = item->next;
	}

	if (item == &devices_list) {
		printf("DEVMAP: no device named %s.\n", name);
		return NULL;
	}

	device = list_get_instance(item, devmap_device_t, devices);
	return device;
}

/** Find device with given handle.
 * @todo: use hash table
 */
static devmap_device_t *devmap_device_find_handle(int handle)
{
	link_t *item;
	devmap_device_t *device = NULL;
	
	futex_down(&devices_list_futex);

	item = (&devices_list)->next;

	while (item != &devices_list) {

		device = list_get_instance(item, devmap_device_t, devices);
		if (device->handle == handle) {
			break;
		}
		item = item->next;
	}

	if (item == &devices_list) {
		futex_up(&devices_list_futex);
		return NULL;
	}

	device = list_get_instance(item, devmap_device_t, devices);
	
	futex_up(&devices_list_futex);

	return device;
}

/**
 * Unregister device and free it. It's assumed that driver's device list is
 * already locked.
 */
static int devmap_device_unregister_core(devmap_device_t *device)
{

	list_remove(&(device->devices));
	list_remove(&(device->driver_devices));

	free(device->name);	
	free(device);


	return EOK;
}

/**
 * Read info about new driver and add it into linked list of registered
 * drivers.
 */
static void devmap_driver_register(devmap_driver_t **odriver)
{
	size_t name_size;
	ipc_callid_t callid;
	ipc_call_t call;
	devmap_driver_t *driver;
	ipc_callid_t iid;
	ipc_call_t icall;

	*odriver = NULL;
	
	iid = async_get_call(&icall);

	if (IPC_GET_METHOD(icall) != DEVMAP_DRIVER_REGISTER) {
		ipc_answer_0(iid, EREFUSED);
		return;
	} 

	if (NULL ==
	    (driver = (devmap_driver_t *)malloc(sizeof(devmap_driver_t)))) {
		ipc_answer_0(iid, ENOMEM);
		return;
	}

	/* 
	 * Get driver name
	 */
	if (!ipc_data_receive(&callid, NULL, &name_size)) {
		printf("Unexpected request.\n");
		free(driver);
		ipc_answer_0(callid, EREFUSED);
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	if (name_size > DEVMAP_NAME_MAXLEN) {
		printf("Too logn name: %u: maximum is %u.\n", name_size,
		    DEVMAP_NAME_MAXLEN);
		free(driver);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	/*
	 * Allocate buffer for device name.
	 */
	if (NULL == (driver->name = (char *)malloc(name_size + 1))) {
		printf("Cannot allocate space for driver name.\n");
		free(driver);
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}	

	/*
	 * Send confirmation to sender and get data into buffer.
	 */
	if (EOK != ipc_data_deliver(callid, driver->name, name_size)) {
		printf("Cannot read driver name.\n");
		free(driver->name);
		free(driver);
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	driver->name[name_size] = 0;

	printf("Read driver name: '%s'.\n", driver->name);

	/* Initialize futex for list of devices owned by this driver */
	futex_initialize(&(driver->devices_futex), 1);

	/* 
	 * Initialize list of asociated devices
	 */
	list_initialize(&(driver->devices));

	/* 
	 * Create connection to the driver 
	 */
	callid = async_get_call(&call);

	if (IPC_M_CONNECT_TO_ME != IPC_GET_METHOD(call)) {
		printf("DEVMAP: Unexpected method: %u.\n",
		    IPC_GET_METHOD(call));
		ipc_answer_0(callid, ENOTSUP);
		
		free(driver->name);
		free(driver);
		ipc_answer_0(iid, ENOTSUP);
		return;
	}

	driver->phone = IPC_GET_ARG3(call);
	
	ipc_answer_0(callid, EOK);
	
	list_initialize(&(driver->drivers));

	futex_down(&drivers_list_futex);	
	
	/* TODO:
	 * check that no driver with name equal to driver->name is registered
	 */

	/* 
	 * Insert new driver into list of registered drivers
	 */
	list_append(&(driver->drivers), &drivers_list);
	futex_up(&drivers_list_futex);	
	
	ipc_answer_0(iid, EOK);
	printf("Driver registered.\n");

	*odriver = driver;
	return;
}

/** Unregister device driver, unregister all its devices and free driver
 * structure.
 */
static int devmap_driver_unregister(devmap_driver_t *driver)
{
	devmap_device_t *device;

	if (NULL == driver) {
		printf("Error: driver == NULL.\n");
		return EEXISTS;
	}

	printf("Unregister driver '%s'.\n", driver->name);
	futex_down(&drivers_list_futex);	

	ipc_hangup(driver->phone);
	
	/* remove it from list of drivers */
	list_remove(&(driver->drivers));

	/* unregister all its devices */
	
	futex_down(&devices_list_futex);	
	futex_down(&(driver->devices_futex));

	while (!list_empty(&(driver->devices))) {
		device = list_get_instance(driver->devices.next,
		    devmap_device_t, driver_devices);
		printf("Unregister device '%s'.\n", device->name);
		devmap_device_unregister_core(device);
	}
	
	futex_up(&(driver->devices_futex));
	futex_up(&devices_list_futex);	
	futex_up(&drivers_list_futex);	

	/* free name and driver */
	if (NULL != driver->name) {
		free(driver->name);
	}

	free(driver);

	printf("Driver unregistered.\n");

	return EOK;
}


/** Register instance of device
 *
 */
static void devmap_device_register(ipc_callid_t iid, ipc_call_t *icall,
    devmap_driver_t *driver)
{
	ipc_callid_t callid;
	size_t size;
	devmap_device_t *device;

	if (NULL == driver) {
		printf("Invalid driver registration.\n");
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/* Create new device entry */
	if (NULL ==
	    (device = (devmap_device_t *)malloc(sizeof(devmap_device_t)))) {
		printf("Cannot allocate new device.\n");
		ipc_answer_0(iid, ENOMEM);
		return;
	}
	
	/* Get device name */
	if (!ipc_data_receive(&callid, NULL, &size)) {
		free(device);
		printf("Cannot read device name.\n");
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	if (size > DEVMAP_NAME_MAXLEN) {
		printf("Too long device name: %u.\n", size);
		free(device);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}

		/* +1 for terminating \0 */
	device->name = (char *)malloc(size + 1);

	if (NULL == device->name) {
		printf("Cannot read device name.\n");
		free(device);
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	ipc_data_deliver(callid, device->name, size);
	device->name[size] = 0;

	list_initialize(&(device->devices));
	list_initialize(&(device->driver_devices));

	futex_down(&devices_list_futex);	

	/* Check that device with such name is not already registered */
	if (NULL != devmap_device_find_name(device->name)) {
		printf("Device '%s' already registered.\n", device->name);
		futex_up(&devices_list_futex);	
		free(device->name);
		free(device);
		ipc_answer_0(iid, EEXISTS);
		return;
	}

	/* Get unique device handle */
	device->handle = devmap_create_handle(); 

	device->driver = driver;
	
	/* Insert device into list of all devices  */
	list_append(&device->devices, &devices_list);

	/* Insert device into list of devices that belog to one driver */
	futex_down(&device->driver->devices_futex);	
	
	list_append(&device->driver_devices, &device->driver->devices);
	
	futex_up(&device->driver->devices_futex);	
	futex_up(&devices_list_futex);	

	printf("Device '%s' registered.\n", device->name);	
	ipc_answer_1(iid, EOK, device->handle);

	return;
}

/**
 *
 */
static int devmap_device_unregister(ipc_callid_t iid, ipc_call_t *icall, 
    devmap_driver_t *driver)
{
	/* TODO */

	return EOK;
}

/** Connect client to the device.
 * Find device driver owning requested device and forward
 * the message to it.
 *
 *
 */
static void devmap_forward(ipc_callid_t callid, ipc_call_t *call)
{
	devmap_device_t *dev;
	int handle;

	/*
	 * Get handle from request
	 */
	handle = IPC_GET_ARG1(*call);
	dev = devmap_device_find_handle(handle);

	if (NULL == dev) {
		printf("DEVMAP: No registered device with handle %d.\n",
		    handle);
		ipc_answer_0(callid, ENOENT);
		return;
	} 

	/* FIXME: is this correct method how to pass argument on forwarding ?*/
	ipc_forward_fast(callid, dev->driver->phone, (ipcarg_t)(dev->handle),
	    0, IPC_FF_NONE);
	return;
}

/** Find handle for device instance identified by name.
 * In answer will be send EOK and device handle in arg1 or a error
 * code from errno.h. 
 */
static void devmap_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	char *name = NULL;
	size_t name_size;
	const devmap_device_t *dev;
	ipc_callid_t callid;
	ipcarg_t retval;
	
	
	/* 
	 * Wait for incoming message with device name (but do not
	 * read the name itself until the buffer is allocated).
	 */
	if (!ipc_data_receive(&callid, NULL, &name_size)) {
		ipc_answer_0(callid, EREFUSED);
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	if (name_size > DEVMAP_NAME_MAXLEN) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	/*
	 * Allocate buffer for device name.
	 */
	if (NULL == (name = (char *)malloc(name_size))) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}	

	/*
	 * Send confirmation to sender and get data into buffer.
	 */
	if (EOK != (retval = ipc_data_deliver(callid, name, name_size))) {
		ipc_answer_0(iid, EREFUSED);
		return;
	}

	/*
	 * Find device name in linked list of known devices.
	 */
	dev = devmap_device_find_name(name);

	/*
	 * Device was not found.
	 */
	if (NULL == dev) {
		printf("DEVMAP: device %s has not been registered.\n", name);
		ipc_answer_0(iid, ENOENT);
		return;
	}

	printf("DEVMAP: device %s has handler %d.\n", name, dev->handle);
		
	ipc_answer_1(iid, EOK, dev->handle);

	return;
}

/** Find name of device identified by id and send it to caller. 
 *
 */
static void devmap_get_name(ipc_callid_t iid, ipc_call_t *icall) 
{
	const devmap_device_t *device;
	size_t name_size;

	device = devmap_device_find_handle(IPC_GET_ARG1(*icall));

	/*
	 * Device not found.
	 */
	if (NULL == device) {
		ipc_answer_0(iid, ENOENT);
		return;
	}	

	ipc_answer_0(iid, EOK);

	name_size = strlen(device->name);


/*	FIXME:
	we have no channel from DEVMAP to client -> 
	sending must be initiated by client

	int rc = ipc_data_send(phone, device->name, name_size); 
	if (rc != EOK) {
		async_wait_for(req, NULL);
		return rc;
	}
*/	
	/* TODO: send name in response */

	return;
}

/** Handle connection with device driver.
 *
 */
static void
devmap_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	bool cont = true;
	devmap_driver_t *driver = NULL; 

	ipc_answer_0(iid, EOK); 

	devmap_driver_register(&driver);

	if (NULL == driver) {
		printf("DEVMAP: driver registration failed.\n");
		return;
	}
	
	while (cont) {
		callid = async_get_call(&call);

 		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			printf("DEVMAP: connection hung up.\n");
			cont = false;
			continue; /* Exit thread */
		case DEVMAP_DRIVER_UNREGISTER:
			printf("DEVMAP: unregister driver.\n");
			if (NULL == driver) {
				printf("DEVMAP: driver was not registered!\n");
				ipc_answer_0(callid, ENOENT);
			} else {
				ipc_answer_0(callid, EOK);
			}
			break;
		case DEVMAP_DEVICE_REGISTER:
			/* Register one instance of device */
			devmap_device_register(callid, &call, driver);
			break;
		case DEVMAP_DEVICE_UNREGISTER:
			/* Remove instance of device identified by handler */
			devmap_device_unregister(callid, &call, driver);
			break;
		case DEVMAP_DEVICE_GET_HANDLE:
			devmap_get_handle(callid, &call);
			break;
		case DEVMAP_DEVICE_GET_NAME:
			devmap_get_handle(callid, &call);
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION)) {
				ipc_answer_0(callid, ENOENT);
			}
		}
	}
	
	if (NULL != driver) {
		/* 
		 * Unregister the device driver and all its devices.
		 */
		devmap_driver_unregister(driver);
		driver = NULL;
	}
	
}

/** Handle connection with device client.
 *
 */
static void
devmap_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	bool cont = true;

	ipc_answer_0(iid, EOK); /* Accept connection */

	while (cont) {
		callid = async_get_call(&call);

 		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			printf("DEVMAP: connection hung up.\n");
			cont = false;
			continue; /* Exit thread */

		case DEVMAP_DEVICE_CONNECT_ME_TO:
			/* Connect client to selected device */
			printf("DEVMAP: connect to device %d.\n",
			    IPC_GET_ARG1(call));
			devmap_forward(callid, &call);
			break;

		case DEVMAP_DEVICE_GET_HANDLE:
 			devmap_get_handle(callid, &call);

			break;
		case DEVMAP_DEVICE_GET_NAME:
			/* TODO */
			devmap_get_name(callid, &call);
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION)) {
				ipc_answer_0(callid, ENOENT);
			}
		}
	}
}

/** Function for handling connections to devmap 
 *
 */
static void
devmap_connection(ipc_callid_t iid, ipc_call_t *icall)
{

	printf("DEVMAP: new connection.\n");

		/* Select interface */
	switch ((ipcarg_t)(IPC_GET_ARG1(*icall))) {
	case DEVMAP_DRIVER:
		devmap_connection_driver(iid, icall);
		break;
	case DEVMAP_CLIENT:
		devmap_connection_client(iid, icall);
		break;
	default:
		ipc_answer_0(iid, ENOENT); /* No such interface */
		printf("DEVMAP: Unknown interface %u.\n",
		    (ipcarg_t)(IPC_GET_ARG1(*icall)));
	}

	/* Cleanup */
	
	printf("DEVMAP: connection closed.\n");
	return;
}

/**
 *
 */
int main(int argc, char *argv[])
{
	ipcarg_t phonead;

	printf("DEVMAP: HelenOS device mapper.\n");

	if (devmap_init() != 0) {
		printf("Error while initializing DEVMAP service.\n");
		return -1;
	}

		/* Set a handler of incomming connections */
	async_set_client_connection(devmap_connection);

	/* Register device mapper at naming service */
	if (ipc_connect_to_me(PHONE_NS, SERVICE_DEVMAP, 0, &phonead) != 0) 
		return -1;
	
	async_manager();
	/* Never reached */
	return 0;
}

/** 
 * @}
 */

