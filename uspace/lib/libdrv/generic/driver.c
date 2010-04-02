/*
 * Copyright (c) 2010 Lenka Trochtova
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
 * @defgroup libdrv generic device driver support.
 * @brief HelenOS generic device driver support.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <ipc/services.h>
#include <ipc/ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <devman.h>
#include <ipc/devman.h>
#include <ipc/driver.h>

#include "driver.h"

static driver_t *driver;
LIST_INITIALIZE(devices);

static device_t * driver_create_device()
{
	device_t *dev = (device_t *)malloc(sizeof(device_t));
	if (NULL != dev) {
		memset(dev, 0, sizeof(device_t));
	}
	return dev;
}

static device_t * driver_get_device(link_t *devices, device_handle_t handle)
{
	device_t *dev = NULL;
	link_t *link = devices->next;

	while (link != devices) {
		dev = list_get_instance(link, device_t, link);
		if (handle == dev->handle) {
			return dev;
		}
	}

	return NULL;
}

static void driver_add_device(ipc_callid_t iid, ipc_call_t *icall)
{
	printf("%s: driver_add_device\n", driver->name);

	// result of the operation - device was added, device is not present etc.
	ipcarg_t ret = 0;
	device_handle_t dev_handle =  IPC_GET_ARG1(*icall);
	device_t *dev = driver_create_device();
	dev->handle = dev_handle;
	if (driver->driver_ops->add_device(dev)) {
		list_append(&dev->link, &devices);
		// TODO set return value
	}
	printf("%s: new device with handle = %x was added.\n", driver->name, dev_handle);

	ipc_answer_1(iid, EOK, ret);
}

static void driver_connection_devman(ipc_callid_t iid, ipc_call_t *icall)
{
	printf("%s: driver_connection_devman \n", driver->name);

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
		case DRIVER_ADD_DEVICE:
			driver_add_device(callid, &call);
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION))
				ipc_answer_0(callid, ENOENT);
		}
	}
}

/**
 * Generic client connection handler both for applications and drivers.
 *
 * @param driver true for driver client, false for other clients (applications, services etc.).
 */
static void driver_connection_gen(ipc_callid_t iid, ipc_call_t *icall, bool driver)
{
	// Answer the first IPC_M_CONNECT_ME_TO call and remember the handle of the device to which the client connected.
	device_handle_t handle = IPC_GET_ARG1(*icall);
	device_t *dev = driver_get_device(&devices, handle);

	if (dev == NULL) {
		ipc_answer_0(iid, ENOENT);
		return;
	}
	
	
	// TODO - if the client is not a driver, check whether it is allowed to use the device

	// TODO open the device (introduce some callbacks for opening and closing devices registered by the driver)
	
	ipc_answer_0(iid, EOK);

	while (1) {
		ipc_callid_t callid;
		ipc_call_t call;

		callid = async_get_call(&call);
		ipcarg_t method = IPC_GET_METHOD(call);
		switch  (method) {
		case IPC_M_PHONE_HUNGUP:
		
			// TODO close the device
			
			ipc_answer_0(callid, EOK);
			return;
		default:

			if (!is_valid_iface_id(method)) {
				// this is not device's interface
				ipc_answer_0(callid, ENOTSUP);
				break;
			}

			// calling one of the device's interfaces
			
			// get the device interface structure
			void *iface = device_get_iface(dev, method);
			if (NULL == iface) {
				ipc_answer_0(callid, ENOTSUP);
				break;
			}

			// get the corresponding interface for remote request handling ("remote interface")
			remote_iface_t* rem_iface = get_remote_iface(method);
			assert(NULL != rem_iface);

			// get the method of the remote interface
			ipcarg_t iface_method_idx = IPC_GET_ARG1(call);
			remote_iface_func_ptr_t iface_method_ptr = get_remote_method(rem_iface, iface_method_idx);
			if (NULL == iface_method_ptr) {
				// the interface has not such method
				ipc_answer_0(callid, ENOTSUP);
				break;
			}
			
			// call the remote interface's method, which will receive parameters from the remote client
			// and it will pass it to the corresponding local interface method associated with the device 
			// by its driver
			(*iface_method_ptr)(dev, iface, callid, &call);
			break;
		}
	}
}

static void driver_connection_driver(ipc_callid_t iid, ipc_call_t *icall)
{
	driver_connection_gen(iid, icall, true);
}

static void driver_connection_client(ipc_callid_t iid, ipc_call_t *icall)
{
	driver_connection_gen(iid, icall, false);
}


/** Function for handling connections to device driver.
 *
 */
static void driver_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Select interface */
	switch ((ipcarg_t) (IPC_GET_ARG1(*icall))) {
	case DRIVER_DEVMAN:
		// handle PnP events from device manager
		driver_connection_devman(iid, icall);
		break;
	case DRIVER_DRIVER:
		// handle request from drivers of child devices
		driver_connection_driver(iid, icall);
		break;
	case DRIVER_CLIENT:
		// handle requests from client applications
		driver_connection_client(iid, icall);
		break;

	default:
		/* No such interface */
		ipc_answer_0(iid, ENOENT);
	}
}

bool child_device_register(device_t *child, device_t *parent)
{
	printf("%s: child_device_register\n", driver->name);

	assert(NULL != child->name);

	if (EOK == devman_child_device_register(child->name, &child->match_ids, parent->handle, &child->handle)) {
		return true;
	}
	return false;
}

int driver_main(driver_t *drv)
{
	// remember the driver structure - driver_ops will be called by generic handler for incoming connections
	driver = drv;

	// register driver by device manager with generic handler for incoming connections
	devman_driver_register(driver->name, driver_connection);

	async_manager();

	// Never reached
	return 0;
}

/**
 * @}
 */
