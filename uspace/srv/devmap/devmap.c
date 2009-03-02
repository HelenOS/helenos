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
 * @brief HelenOS device mapper.
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
#include <ipc/devmap.h>

#define NAME  "devmap"

/** Pending lookup structure. */
typedef struct {
	link_t link;
	char *name;              /**< Device name */
	ipc_callid_t callid;     /**< Call ID waiting for the lookup */
} pending_req_t;

LIST_INITIALIZE(devices_list);
LIST_INITIALIZE(drivers_list);
LIST_INITIALIZE(pending_req);

/* Locking order:
 *  drivers_list_futex
 *  devices_list_futex
 *  (devmap_driver_t *)->devices_futex
 *  create_handle_futex
 **/

static atomic_t devices_list_futex = FUTEX_INITIALIZER;
static atomic_t drivers_list_futex = FUTEX_INITIALIZER;
static atomic_t create_handle_futex = FUTEX_INITIALIZER;

static int devmap_create_handle(void)
{
	static int last_handle = 0;
	int handle;
	
	/* TODO: allow reusing old handles after their unregistration
	 * and implement some version of LRU algorithm
	 */
	
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
	link_t *item = devices_list.next;
	devmap_device_t *device = NULL;
	
	while (item != &devices_list) {
		device = list_get_instance(item, devmap_device_t, devices);
		if (0 == strcmp(device->name, name))
			break;
		item = item->next;
	}
	
	if (item == &devices_list)
		return NULL;
	
	device = list_get_instance(item, devmap_device_t, devices);
	return device;
}

/** Find device with given handle.
 *
 * @todo: use hash table
 *
 */
static devmap_device_t *devmap_device_find_handle(int handle)
{
	futex_down(&devices_list_futex);
	
	link_t *item = (&devices_list)->next;
	devmap_device_t *device = NULL;
	
	while (item != &devices_list) {
		device = list_get_instance(item, devmap_device_t, devices);
		if (device->handle == handle)
			break;
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
 *
 * Unregister device and free it. It's assumed that driver's device list is
 * already locked.
 *
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
 *
 * Read info about new driver and add it into linked list of registered
 * drivers.
 *
 */
static void devmap_driver_register(devmap_driver_t **odriver)
{
	*odriver = NULL;
	
	ipc_call_t icall;
	ipc_callid_t iid = async_get_call(&icall);
	
	if (IPC_GET_METHOD(icall) != DEVMAP_DRIVER_REGISTER) {
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	devmap_driver_t *driver = (devmap_driver_t *) malloc(sizeof(devmap_driver_t));
	
	if (driver == NULL) {
		ipc_answer_0(iid, ENOMEM);
		return;
	}
	
	/*
	 * Get driver name
	 */
	ipc_callid_t callid;
	size_t name_size;
	if (!ipc_data_write_receive(&callid, &name_size)) {
		free(driver);
		ipc_answer_0(callid, EREFUSED);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	if (name_size > DEVMAP_NAME_MAXLEN) {
		free(driver);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/*
	 * Allocate buffer for device name.
	 */
	driver->name = (char *) malloc(name_size + 1);
	if (driver->name == NULL) {
		free(driver);
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/*
	 * Send confirmation to sender and get data into buffer.
	 */
	if (EOK != ipc_data_write_finalize(callid, driver->name, name_size)) {
		free(driver->name);
		free(driver);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	driver->name[name_size] = 0;
	
	/* Initialize futex for list of devices owned by this driver */
	futex_initialize(&(driver->devices_futex), 1);
	
	/*
	 * Initialize list of asociated devices
	 */
	list_initialize(&(driver->devices));
	
	/*
	 * Create connection to the driver 
	 */
	ipc_call_t call;
	callid = async_get_call(&call);
	
	if (IPC_M_CONNECT_TO_ME != IPC_GET_METHOD(call)) {
		ipc_answer_0(callid, ENOTSUP);
		
		free(driver->name);
		free(driver);
		ipc_answer_0(iid, ENOTSUP);
		return;
	}
	
	driver->phone = IPC_GET_ARG5(call);
	
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
	
	*odriver = driver;
}

/**
 * Unregister device driver, unregister all its devices and free driver
 * structure.
 *
 */
static int devmap_driver_unregister(devmap_driver_t *driver)
{
	if (driver == NULL)
		return EEXISTS;
	
	futex_down(&drivers_list_futex);
	
	ipc_hangup(driver->phone);
	
	/* remove it from list of drivers */
	list_remove(&(driver->drivers));
	
	/* unregister all its devices */
	
	futex_down(&devices_list_futex);
	futex_down(&(driver->devices_futex));
	
	while (!list_empty(&(driver->devices))) {
		devmap_device_t *device = list_get_instance(driver->devices.next,
		    devmap_device_t, driver_devices);
		devmap_device_unregister_core(device);
	}
	
	futex_up(&(driver->devices_futex));
	futex_up(&devices_list_futex);
	futex_up(&drivers_list_futex);
	
	/* free name and driver */
	if (NULL != driver->name)
		free(driver->name);
	
	free(driver);
	
	return EOK;
}


/** Process pending lookup requests */
static void process_pending_lookup()
{
	link_t *cur;
	
loop:
	for (cur = pending_req.next; cur != &pending_req; cur = cur->next) {
		pending_req_t *pr = list_get_instance(cur, pending_req_t, link);
		
		const devmap_device_t *dev = devmap_device_find_name(pr->name);
		if (!dev)
			continue;
		
		ipc_answer_1(pr->callid, EOK, dev->handle);
		
		free(pr->name);
		list_remove(cur);
		free(pr);
		goto loop;
	}
}


/** Register instance of device
 *
 */
static void devmap_device_register(ipc_callid_t iid, ipc_call_t *icall,
    devmap_driver_t *driver)
{
	if (driver == NULL) {
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/* Create new device entry */
	devmap_device_t *device = (devmap_device_t *) malloc(sizeof(devmap_device_t));
	if (device == NULL) {
		ipc_answer_0(iid, ENOMEM);
		return;
	}
	
	/* Get device name */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		free(device);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size > DEVMAP_NAME_MAXLEN) {
		free(device);
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/* +1 for terminating \0 */
	device->name = (char *) malloc(size + 1);
	
	if (device->name == NULL) {
		free(device);
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	ipc_data_write_finalize(callid, device->name, size);
	device->name[size] = 0;
	
	list_initialize(&(device->devices));
	list_initialize(&(device->driver_devices));
	
	futex_down(&devices_list_futex);
	
	/* Check that device with such name is not already registered */
	if (NULL != devmap_device_find_name(device->name)) {
		printf(NAME ": Device '%s' already registered\n", device->name);
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
	
	ipc_answer_1(iid, EOK, device->handle);
	
	process_pending_lookup();
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
 *
 * Find device driver owning requested device and forward
 * the message to it.
 *
 */
static void devmap_forward(ipc_callid_t callid, ipc_call_t *call)
{
	/*
	 * Get handle from request
	 */
	int handle = IPC_GET_ARG2(*call);
	devmap_device_t *dev = devmap_device_find_handle(handle);
	
	if (NULL == dev) {
		ipc_answer_0(callid, ENOENT);
		return;
	}
	
	ipc_forward_fast(callid, dev->driver->phone, (ipcarg_t)(dev->handle),
	    IPC_GET_ARG3(*call), 0, IPC_FF_NONE);
}

/** Find handle for device instance identified by name.
 *
 * In answer will be send EOK and device handle in arg1 or a error
 * code from errno.h.
 *
 */
static void devmap_get_handle(ipc_callid_t iid, ipc_call_t *icall)
{
	/*
	 * Wait for incoming message with device name (but do not
	 * read the name itself until the buffer is allocated).
	 */
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EREFUSED);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	if ((size < 1) || (size > DEVMAP_NAME_MAXLEN)) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/*
	 * Allocate buffer for device name.
	 */
	char *name = (char *) malloc(size);
	if (name == NULL) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	/*
	 * Send confirmation to sender and get data into buffer.
	 */
	ipcarg_t retval = ipc_data_write_finalize(callid, name, size);
	if (retval != EOK) {
		ipc_answer_0(iid, EREFUSED);
		free(name);
		return;
	}
	name[size] = '\0';
	
	/*
	 * Find device name in linked list of known devices.
	 */
	const devmap_device_t *dev = devmap_device_find_name(name);
	
	/*
	 * Device was not found.
	 */
	if (dev == NULL) {
		if (IPC_GET_ARG1(*icall) & IPC_FLAG_BLOCKING) {
			/* Blocking lookup, add to pending list */
			pending_req_t *pr = (pending_req_t *) malloc(sizeof(pending_req_t));
			if (!pr) {
				ipc_answer_0(iid, ENOMEM);
				free(name);
				return;
			}
			
			pr->name = name;
			pr->callid = iid;
			list_append(&pr->link, &pending_req);
			return;
		}
		
		ipc_answer_0(iid, ENOENT);
		free(name);
		return;
	}
	
	ipc_answer_1(iid, EOK, dev->handle);
	free(name);
}

/** Find name of device identified by id and send it to caller.
 *
 */
static void devmap_get_name(ipc_callid_t iid, ipc_call_t *icall) 
{
	const devmap_device_t *device = devmap_device_find_handle(IPC_GET_ARG1(*icall));
	
	/*
	 * Device not found.
	 */
	if (device == NULL) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	ipc_answer_0(iid, EOK);
	
	size_t name_size = strlen(device->name);
	
	/* FIXME:
	 * We have no channel from DEVMAP to client, therefore
	 * sending must be initiated by client.
	 *
	 * int rc = ipc_data_write_send(phone, device->name, name_size);
	 * if (rc != EOK) {
	 *     async_wait_for(req, NULL);
	 *     return rc;
	 * }
	 */
	
	/* TODO: send name in response */
}

/** Handle connection with device driver.
 *
 */
static void devmap_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection */
	ipc_answer_0(iid, EOK);
	
	devmap_driver_t *driver = NULL; 
	devmap_driver_register(&driver);
	
	if (NULL == driver)
		return;
	
	bool cont = true;
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			/* Exit thread */
			continue;
		case DEVMAP_DRIVER_UNREGISTER:
			if (NULL == driver)
				ipc_answer_0(callid, ENOENT);
			else
				ipc_answer_0(callid, EOK);
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
			if (!(callid & IPC_CALLID_NOTIFICATION))
				ipc_answer_0(callid, ENOENT);
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
static void devmap_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection */
	ipc_answer_0(iid, EOK);
	
	bool cont = true;
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			/* Exit thread */
			continue;
		case DEVMAP_DEVICE_GET_HANDLE:
			devmap_get_handle(callid, &call);
			break;
		case DEVMAP_DEVICE_GET_NAME:
			/* TODO */
			devmap_get_name(callid, &call);
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION))
				ipc_answer_0(callid, ENOENT);
		}
	}
}

/** Function for handling connections to devmap
 *
 */
static void devmap_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Select interface */
	switch ((ipcarg_t) (IPC_GET_ARG1(*icall))) {
	case DEVMAP_DRIVER:
		devmap_connection_driver(iid, icall);
		break;
	case DEVMAP_CLIENT:
		devmap_connection_client(iid, icall);
		break;
	case DEVMAP_CONNECT_TO_DEVICE:
		/* Connect client to selected device */
		devmap_forward(iid, icall);
		break;
	default:
		/* No such interface */
		ipc_answer_0(iid, ENOENT); 
	}
}

/**
 *
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Device Mapper\n");
	
	if (devmap_init() != 0) {
		printf(NAME ": Error while initializing service\n");
		return -1;
	}
	
	/* Set a handler of incomming connections */
	async_set_client_connection(devmap_connection);
	
	/* Register device mapper at naming service */
	ipcarg_t phonead;
	if (ipc_connect_to_me(PHONE_NS, SERVICE_DEVMAP, 0, 0, &phonead) != 0) 
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();
	
	/* Never reached */
	return 0;
}

/** 
 * @}
 */
