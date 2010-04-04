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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */
#ifndef LIBDRV_DRIVER_H_
#define LIBDRV_DRIVER_H_


#include <adt/list.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include <assert.h>

struct device;
typedef struct device device_t;

// device interface

// first two parameters: device and interface structure registered by the devices driver
typedef void remote_iface_func_t(device_t*, void *, ipc_callid_t, ipc_call_t *);
typedef remote_iface_func_t *remote_iface_func_ptr_t;

typedef struct {
	size_t method_count;
	remote_iface_func_ptr_t *methods;
} remote_iface_t;

typedef struct {
	remote_iface_t * ifaces[DEV_IFACE_COUNT];
} iface_dipatch_table_t;

static inline int get_iface_index(dev_inferface_id_t id)
{
	return id - DEV_IFACE_FIRST;
}

static inline bool is_valid_iface_id(dev_inferface_id_t id)
{
	return DEV_IFACE_FIRST <= id && id < DEV_IFACE_MAX;
}

remote_iface_t* get_remote_iface(dev_inferface_id_t id);
remote_iface_func_ptr_t get_remote_method(remote_iface_t *rem_iface, ipcarg_t iface_method_idx);


// device

/** The device. */
struct device {
	/** Globally unique device identifier (assigned to the device by the device manager). */
	device_handle_t handle;
	/** The phone to the parent device driver.*/
	int parent_phone;
	/** The device's name.*/
	const char *name;
	/** The list of device ids for device-to-driver matching.*/
	match_id_list_t match_ids;
	/** The device driver's data associated with this device.*/
	void *driver_data;
	/** The table of interfaces exported by this device. */
	void *interfaces[DEV_IFACE_COUNT];
	/** Pointer to the previous and next device in the list of devices handled by the driver */
	link_t link;
};


// driver

/** Generic device driver operations. */
typedef struct driver_ops {
	/** Callback method for passing a new device to the device driver.*/
	bool (*add_device)(device_t *dev);
	// TODO add other generic driver operations
} driver_ops_t;

/** The driver structure.*/
typedef struct driver {
	/** The name of the device driver. */
	const char *name;
	/** Generic device driver operations. */
	driver_ops_t *driver_ops;
} driver_t;





int driver_main(driver_t *drv);

/** Create new device structure. 
 * 
 * @return the device structure.
 */
static inline device_t * create_device()
{
	device_t *dev = malloc(sizeof(device_t));
	if (NULL != dev) {
		memset(dev, 0, sizeof(device_t));
	}
	list_initialize(&dev->match_ids.ids);
	return dev;
}

/** Delete device structure. 
 * 
 * @param dev the device structure.
 */
static inline void delete_device(device_t *dev)
{
	clean_match_ids(&dev->match_ids);
	if (NULL != dev->name) {
		free(dev->name);
	}
	free(dev);
}

static inline void device_set_iface (device_t *dev, dev_inferface_id_t id, void *iface)
{
	assert(is_valid_iface_id(id));

	int idx = get_iface_index(id);
	dev->interfaces[idx] = iface;
}

static inline void * device_get_iface(device_t *dev, dev_inferface_id_t id)
{
	assert(is_valid_iface_id(id));
	
	int idx = get_iface_index(id);
	return dev->interfaces[idx];	
}

bool child_device_register(device_t *child, device_t *parent);


#endif

/**
 * @}
 */
