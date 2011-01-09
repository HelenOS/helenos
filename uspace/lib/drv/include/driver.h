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
#include <devman.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include <assert.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <fibril_synch.h>
#include <malloc.h>

struct device;
typedef struct device device_t;

/*
 * Device interface
 */

/*
 * First two parameters: device and interface structure registered by the
 * devices driver.
 */
typedef void remote_iface_func_t(device_t *, void *, ipc_callid_t,
    ipc_call_t *);
typedef remote_iface_func_t *remote_iface_func_ptr_t;
typedef void remote_handler_t(device_t *, ipc_callid_t, ipc_call_t *);

typedef struct {
	size_t method_count;
	remote_iface_func_ptr_t *methods;
} remote_iface_t;

typedef struct {
	remote_iface_t *ifaces[DEV_IFACE_COUNT];
} iface_dipatch_table_t;

extern remote_iface_t *get_remote_iface(int);
extern remote_iface_func_ptr_t get_remote_method(remote_iface_t *, sysarg_t);

/*
 * Device class
 */

/** Devices operations */
typedef struct device_ops {
	/**
	 * Optional callback function called when a client is connecting to the
	 * device.
	 */
	int (*open)(device_t *);
	
	/**
	 * Optional callback function called when a client is disconnecting from
	 * the device.
	 */
	void (*close)(device_t *);
	
	/** The table of standard interfaces implemented by the device. */
	void *interfaces[DEV_IFACE_COUNT];
	
	/**
	 * The default handler of remote client requests. If the client's remote
	 * request cannot be handled by any of the standard interfaces, the
	 * default handler is used.
	 */
	remote_handler_t *default_handler;
} device_ops_t;


/*
 * Device
 */

/** Device structure */
struct device {
	/**
	 * Globally unique device identifier (assigned to the device by the
	 * device manager).
	 */
	devman_handle_t handle;
	
	/**
	 * Phone to the parent device driver (if it is different from this
	 * driver)
	 */
	int parent_phone;
	
	/** Parent device if handled by this driver, NULL otherwise */
	device_t *parent;
	/** Device name */
	const char *name;
	/** List of device ids for device-to-driver matching */
	match_id_list_t match_ids;
	/** Driver-specific data associated with this device */
	void *driver_data;
	/** The implementation of operations provided by this device */
	device_ops_t *ops;
	
	/** Link in the list of devices handled by the driver */
	link_t link;
};


/*
 * Driver
 */

/** Generic device driver operations */
typedef struct driver_ops {
	/** Callback method for passing a new device to the device driver */
	int (*add_device)(device_t *dev);
	/* TODO: add other generic driver operations */
} driver_ops_t;

/** Driver structure */
typedef struct driver {
	/** Name of the device driver */
	const char *name;
	/** Generic device driver operations */
	driver_ops_t *driver_ops;
} driver_t;

int driver_main(driver_t *);

/** Create new device structure.
 *
 * @return		The device structure.
 */
extern device_t *create_device(void);
extern void delete_device(device_t *);
extern void *device_get_ops(device_t *, dev_inferface_idx_t);

extern int child_device_register(device_t *, device_t *);
extern int child_device_register_wrapper(device_t *, const char *, const char *,
    int);

/*
 * Interrupts
 */

typedef void interrupt_handler_t(device_t *, ipc_callid_t, ipc_call_t *);

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

extern interrupt_context_t *create_interrupt_context(void);
extern void delete_interrupt_context(interrupt_context_t *);
extern void init_interrupt_context_list(interrupt_context_list_t *);
extern void add_interrupt_context(interrupt_context_list_t *,
    interrupt_context_t *);
extern void remove_interrupt_context(interrupt_context_list_t *,
    interrupt_context_t *);
extern interrupt_context_t *find_interrupt_context_by_id(
    interrupt_context_list_t *, int);
extern interrupt_context_t *find_interrupt_context(
    interrupt_context_list_t *, device_t *, int);

extern int register_interrupt_handler(device_t *, int, interrupt_handler_t *,
    irq_code_t *);
extern int unregister_interrupt_handler(device_t *, int);

extern remote_handler_t *device_get_default_handler(device_t *);
extern int add_device_to_class(device_t *, const char *);

#endif

/**
 * @}
 */
