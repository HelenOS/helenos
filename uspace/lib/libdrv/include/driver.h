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
#include <ipc/ipc.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include <device/hw_res.h>
#include <device/char.h>
#include <assert.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <fibril_synch.h>
#include <malloc.h>

struct device;
typedef struct device device_t;

// device interface

// first two parameters: device and interface structure registered by the devices driver
typedef void remote_iface_func_t(device_t*, void *, ipc_callid_t, ipc_call_t *);
typedef remote_iface_func_t *remote_iface_func_ptr_t;
typedef void remote_handler_t(device_t*, ipc_callid_t, ipc_call_t *);

typedef struct {
	size_t method_count;
	remote_iface_func_ptr_t *methods;
} remote_iface_t;

typedef struct {
	remote_iface_t * ifaces[DEV_IFACE_COUNT];
} iface_dipatch_table_t;


static inline bool is_valid_iface_idx(int idx)
{
	return 0 <= idx && idx < DEV_IFACE_MAX;
}

remote_iface_t* get_remote_iface(int idx);
remote_iface_func_ptr_t get_remote_method(remote_iface_t *rem_iface, ipcarg_t iface_method_idx);


// device class

/** Devices belonging to the same class should implement the same set of interfaces.*/
typedef struct device_class {
	/** Unique identification of the class. */
	int id;
	/** Optional callback function called when a client is connecting to the device. */
	int (*open)(device_t *dev);
	/** Optional callback function called when a client is disconnecting from the device. */
	void (*close)(device_t *dev);
	/** The table of standard interfaces implemented by the device. */
	void *interfaces[DEV_IFACE_COUNT];	
	/** The default handler of remote client requests. If the client's remote request cannot be handled 
	 * by any of the standard interfaces, the default handler is used.*/
	remote_handler_t *default_handler;
} device_class_t;


// device

/** The device. */
struct device {
	/** Globally unique device identifier (assigned to the device by the device manager). */
	device_handle_t handle;
	/** The phone to the parent device driver (if it is different from this driver).*/
	int parent_phone;
	/** Parent device if handled by this driver, NULL otherwise. */
	device_t *parent;
	/** The device's name.*/
	const char *name;
	/** The list of device ids for device-to-driver matching.*/
	match_id_list_t match_ids;
	/** The device driver's data associated with this device.*/
	void *driver_data;
	/** Device class consist of class id and table of interfaces supported by the device.*/
	device_class_t *class;
	/** Pointer to the previous and next device in the list of devices handled by the driver */
	link_t link;
};


// driver

/** Generic device driver operations. */
typedef struct driver_ops {
	/** Callback method for passing a new device to the device driver.*/
	int (*add_device)(device_t *dev);
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
		init_match_ids(&dev->match_ids);
	}	
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

static inline void * device_get_iface(device_t *dev, dev_inferface_idx_t idx)
{
	assert(is_valid_iface_idx(idx));	
	if (NULL == dev->class) {
		return NULL;
	}
	return dev->class->interfaces[idx];	
}

int child_device_register(device_t *child, device_t *parent);

// interrupts

typedef void interrupt_handler_t(device_t *dev, ipc_callid_t iid, ipc_call_t *icall);

typedef struct interrupt_context {
	int id;
	device_t *dev;
	int irq;
	interrupt_handler_t *handler;
	link_t link;
} interrupt_context_t;

typedef struct interrupt_context_list {
	int curr_id;
	link_t contexts;
	fibril_mutex_t mutex;
} interrupt_context_list_t;

static inline interrupt_context_t * create_interrupt_context()
{ 
	interrupt_context_t *ctx = (interrupt_context_t *)malloc(sizeof(interrupt_context_t));
	if (NULL != ctx) {
		memset(ctx, 0, sizeof(interrupt_context_t));
	}
	return ctx;
}

static inline void delete_interrupt_context(interrupt_context_t *ctx)
{
	if (NULL != ctx) {
		free(ctx);
	}
}

static inline void init_interrupt_context_list(interrupt_context_list_t *list)
{
	memset(list, 0, sizeof(interrupt_context_list_t));
	fibril_mutex_initialize(&list->mutex);
	list_initialize(&list->contexts);	
}

static inline void add_interrupt_context(interrupt_context_list_t *list, interrupt_context_t *ctx)
{
	fibril_mutex_lock(&list->mutex);
	
	ctx->id = list->curr_id++;
	list_append(&ctx->link, &list->contexts);
	
	fibril_mutex_unlock(&list->mutex);
}

static inline void remove_interrupt_context(interrupt_context_list_t *list, interrupt_context_t *ctx)
{
	fibril_mutex_lock(&list->mutex);
	
	list_remove(&ctx->link);
	
	fibril_mutex_unlock(&list->mutex);
}

static inline interrupt_context_t *find_interrupt_context_by_id(interrupt_context_list_t *list, int id) 
{
	fibril_mutex_lock(&list->mutex);	
	link_t *link = list->contexts.next;
	interrupt_context_t *ctx;
	
	while (link != &list->contexts) {
		ctx = list_get_instance(link, interrupt_context_t, link);
		if (id == ctx->id) {
			fibril_mutex_unlock(&list->mutex);
			return ctx;
		}
		link = link->next;
	}	
	fibril_mutex_unlock(&list->mutex);	
	return NULL;
}

static inline interrupt_context_t *find_interrupt_context(interrupt_context_list_t *list, device_t *dev, int irq) 
{
	fibril_mutex_lock(&list->mutex);	
	link_t *link = list->contexts.next;
	interrupt_context_t *ctx;
	
	while (link != &list->contexts) {
		ctx = list_get_instance(link, interrupt_context_t, link);
		if (irq == ctx->irq && dev == ctx->dev) {
			fibril_mutex_unlock(&list->mutex);
			return ctx;
		}
		link = link->next;
	}	
	fibril_mutex_unlock(&list->mutex);	
	return NULL;
}

int register_interrupt_handler(device_t *dev, int irq, interrupt_handler_t *handler, irq_code_t *pseudocode);
int unregister_interrupt_handler(device_t *dev, int irq);


// default handler for client requests

static inline remote_handler_t * device_get_default_handler(device_t *dev)
{
	if (NULL == dev->class) {
		return NULL;
	}
	
	return dev->class->default_handler;	
}

#endif

/**
 * @}
 */
