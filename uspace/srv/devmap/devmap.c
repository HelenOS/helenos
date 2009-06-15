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
#include <fibril_sync.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/devmap.h>
#include <assert.h>

#define NAME  "devmap"

/** Representation of device driver.
 *
 * Each driver is responsible for a set of devices.
 *
 */
typedef struct {
	/** Pointers to previous and next drivers in linked list */
	link_t drivers;
	/** Pointer to the linked list of devices controlled by this driver */
	link_t devices;
	/** Phone asociated with this driver */
	ipcarg_t phone;
	/** Device driver name */
	char *name;
	/** Fibril mutex for list of devices owned by this driver */
	fibril_mutex_t devices_mutex;
} devmap_driver_t;

/** Info about registered device
 *
 */
typedef struct {
	/** Pointer to the previous and next device in the list of all devices */
	link_t devices;
	/** Pointer to the previous and next device in the list of devices
	    owned by one driver */
	link_t driver_devices;
	/** Unique device identifier  */
	dev_handle_t handle;
	/** Device name */
	char *name;
	/** Device driver handling this device */
	devmap_driver_t *driver;
} devmap_device_t;

/** Pending lookup structure. */
typedef struct {
	link_t link;
	char *name;              /**< Device name */
	ipc_callid_t callid;     /**< Call ID waiting for the lookup */
} pending_req_t;

LIST_INITIALIZE(devices_list);
LIST_INITIALIZE(drivers_list);
LIST_INITIALIZE(pending_req);

static bool pending_new_dev = false;
static FIBRIL_CONDVAR_INITIALIZE(pending_cv);

/* Locking order:
 *  drivers_list_mutex
 *  devices_list_mutex
 *  (devmap_driver_t *)->devices_mutex
 *  create_handle_mutex
 **/

static FIBRIL_MUTEX_INITIALIZE(devices_list_mutex);
static FIBRIL_MUTEX_INITIALIZE(drivers_list_mutex);
static FIBRIL_MUTEX_INITIALIZE(create_handle_mutex);

static dev_handle_t last_handle = 0;

static dev_handle_t devmap_create_handle(void)
{
	/* TODO: allow reusing old handles after their unregistration
	 * and implement some version of LRU algorithm, avoid overflow
	 */
	
	fibril_mutex_lock(&create_handle_mutex);
	last_handle++;
	fibril_mutex_unlock(&create_handle_mutex);
	
	return last_handle;
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
		if (str_cmp(device->name, name) == 0)
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
static devmap_device_t *devmap_device_find_handle(dev_handle_t handle)
{
	fibril_mutex_lock(&devices_list_mutex);
	
	link_t *item = (&devices_list)->next;
	devmap_device_t *device = NULL;
	
	while (item != &devices_list) {
		device = list_get_instance(item, devmap_device_t, devices);
		if (device->handle == handle)
			break;
		item = item->next;
	}
	
	if (item == &devices_list) {
		fibril_mutex_unlock(&devices_list_mutex);
		return NULL;
	}
	
	device = list_get_instance(item, devmap_device_t, devices);
	
	fibril_mutex_unlock(&devices_list_mutex);
	
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
	if (ipc_data_write_finalize(callid, driver->name, name_size) != EOK) {
		free(driver->name);
		free(driver);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	driver->name[name_size] = 0;
	
	/* Initialize mutex for list of devices owned by this driver */
	fibril_mutex_initialize(&driver->devices_mutex);
	
	/*
	 * Initialize list of asociated devices
	 */
	list_initialize(&driver->devices);
	
	/*
	 * Create connection to the driver
	 */
	ipc_call_t call;
	callid = async_get_call(&call);
	
	if (IPC_GET_METHOD(call) != IPC_M_CONNECT_TO_ME) {
		ipc_answer_0(callid, ENOTSUP);
		
		free(driver->name);
		free(driver);
		ipc_answer_0(iid, ENOTSUP);
		return;
	}
	
	driver->phone = IPC_GET_ARG5(call);
	
	ipc_answer_0(callid, EOK);
	
	list_initialize(&(driver->drivers));
	
	fibril_mutex_lock(&drivers_list_mutex);
	
	/* TODO:
	 * check that no driver with name equal to driver->name is registered
	 */
	
	/*
	 * Insert new driver into list of registered drivers
	 */
	list_append(&(driver->drivers), &drivers_list);
	fibril_mutex_unlock(&drivers_list_mutex);
	
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
	
	fibril_mutex_lock(&drivers_list_mutex);
	
	if (driver->phone != 0)
		ipc_hangup(driver->phone);
	
	/* Remove it from list of drivers */
	list_remove(&(driver->drivers));
	
	/* Unregister all its devices */
	fibril_mutex_lock(&devices_list_mutex);
	fibril_mutex_lock(&driver->devices_mutex);
	
	while (!list_empty(&(driver->devices))) {
		devmap_device_t *device = list_get_instance(driver->devices.next,
		    devmap_device_t, driver_devices);
		devmap_device_unregister_core(device);
	}
	
	fibril_mutex_unlock(&driver->devices_mutex);
	fibril_mutex_unlock(&devices_list_mutex);
	fibril_mutex_unlock(&drivers_list_mutex);
	
	/* free name and driver */
	if (driver->name != NULL)
		free(driver->name);
	
	free(driver);
	
	return EOK;
}


/** Process pending lookup requests */
static void process_pending_lookup(void)
{
	link_t *cur;

loop:
	fibril_mutex_lock(&devices_list_mutex);
	while (!pending_new_dev)
		fibril_condvar_wait(&pending_cv, &devices_list_mutex);
rescan:
	for (cur = pending_req.next; cur != &pending_req; cur = cur->next) {
		pending_req_t *pr = list_get_instance(cur, pending_req_t, link);
		
		const devmap_device_t *dev = devmap_device_find_name(pr->name);
		if (!dev)
			continue;
		
		ipc_answer_1(pr->callid, EOK, dev->handle);
		
		free(pr->name);
		list_remove(cur);
		free(pr);
		
		goto rescan;
	}
	pending_new_dev = false;
	fibril_mutex_unlock(&devices_list_mutex);
	goto loop;
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
	
	fibril_mutex_lock(&devices_list_mutex);
	
	/* Check that device with such name is not already registered */
	if (NULL != devmap_device_find_name(device->name)) {
		printf(NAME ": Device '%s' already registered\n", device->name);
		fibril_mutex_unlock(&devices_list_mutex);
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
	fibril_mutex_lock(&device->driver->devices_mutex);
	
	list_append(&device->driver_devices, &device->driver->devices);
	
	fibril_mutex_unlock(&device->driver->devices_mutex);
	pending_new_dev = true;
	fibril_condvar_signal(&pending_cv);
	fibril_mutex_unlock(&devices_list_mutex);
	
	ipc_answer_1(iid, EOK, device->handle);
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
	dev_handle_t handle = IPC_GET_ARG2(*call);
	devmap_device_t *dev = devmap_device_find_handle(handle);
	
	if ((dev == NULL) || (dev->driver == NULL) || (dev->driver->phone == 0)) {
		ipc_answer_0(callid, ENOENT);
		return;
	}
	
	ipc_forward_fast(callid, dev->driver->phone, dev->handle,
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
	char *name = (char *) malloc(size + 1);
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
	
	fibril_mutex_lock(&devices_list_mutex);

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
				fibril_mutex_unlock(&devices_list_mutex);
				ipc_answer_0(iid, ENOMEM);
				free(name);
				return;
			}
			
			pr->name = name;
			pr->callid = iid;
			list_append(&pr->link, &pending_req);
			fibril_mutex_unlock(&devices_list_mutex);
			return;
		}
		
		ipc_answer_0(iid, ENOENT);
		free(name);
		fibril_mutex_unlock(&devices_list_mutex);
		return;
	}
	fibril_mutex_unlock(&devices_list_mutex);
	
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
	
	size_t name_size = str_size(device->name);
	
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

static void devmap_get_count(ipc_callid_t iid, ipc_call_t *icall)
{
	fibril_mutex_lock(&devices_list_mutex);
	ipc_answer_1(iid, EOK, list_count(&devices_list));
	fibril_mutex_unlock(&devices_list_mutex);
}

static void devmap_get_devices(ipc_callid_t iid, ipc_call_t *icall)
{
	fibril_mutex_lock(&devices_list_mutex);
	
	ipc_callid_t callid;
	size_t size;
	if (!ipc_data_read_receive(&callid, &size)) {
		ipc_answer_0(callid, EREFUSED);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	if ((size % sizeof(dev_desc_t)) != 0) {
		ipc_answer_0(callid, EINVAL);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	size_t count = size / sizeof(dev_desc_t);
	dev_desc_t *desc = (dev_desc_t *) malloc(size);
	if (desc == NULL) {
		ipc_answer_0(callid, ENOMEM);
		ipc_answer_0(iid, EREFUSED);
		return;
	}
	
	size_t pos = 0;
	link_t *item = devices_list.next;
	
	while ((item != &devices_list) && (pos < count)) {
		devmap_device_t *device = list_get_instance(item, devmap_device_t, devices);
		
		desc[pos].handle = device->handle;
		str_cpy(desc[pos].name, DEVMAP_NAME_MAXLEN, device->name);
		pos++;
		item = item->next;
	}
	
	ipcarg_t retval = ipc_data_read_finalize(callid, desc, pos * sizeof(dev_desc_t));
	if (retval != EOK) {
		ipc_answer_0(iid, EREFUSED);
		free(desc);
		return;
	}
	
	free(desc);
	
	fibril_mutex_unlock(&devices_list_mutex);
	
	ipc_answer_1(iid, EOK, pos);
}

/** Initialize device mapper.
 *
 *
 */
static bool devmap_init(void)
{
	/* Create NULL device entry */
	devmap_device_t *device = (devmap_device_t *) malloc(sizeof(devmap_device_t));
	if (device == NULL)
		return false;
	
	device->name = str_dup("null");
	if (device->name == NULL) {
		free(device);
		return false;
	}
	
	list_initialize(&(device->devices));
	list_initialize(&(device->driver_devices));
	
	fibril_mutex_lock(&devices_list_mutex);
	
	/* Get unique device handle */
	device->handle = devmap_create_handle();
	device->driver = NULL;
	
	/* Insert device into list of all devices  */
	list_append(&device->devices, &devices_list);
	
	fibril_mutex_unlock(&devices_list_mutex);
	
	return true;
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
			devmap_get_name(callid, &call);
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
			continue;
		case DEVMAP_DEVICE_GET_HANDLE:
			devmap_get_handle(callid, &call);
			break;
		case DEVMAP_DEVICE_GET_NAME:
			devmap_get_name(callid, &call);
			break;
		case DEVMAP_DEVICE_GET_COUNT:
			devmap_get_count(callid, &call);
			break;
		case DEVMAP_DEVICE_GET_DEVICES:
			devmap_get_devices(callid, &call);
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
	
	if (!devmap_init()) {
		printf(NAME ": Error while initializing service\n");
		return -1;
	}
	
	/* Set a handler of incomming connections */
	async_set_client_connection(devmap_connection);

	/* Create a fibril for handling pending device lookups */
	fid_t fid = fibril_create(process_pending_lookup, NULL);
	assert(fid);
	fibril_add_ready(fid);
	
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
