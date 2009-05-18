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

/** @addtogroup devmap
 * @{
 */ 

#ifndef DEVMAP_DEVMAP_H_
#define DEVMAP_DEVMAP_H_

#include <atomic.h>
#include <ipc/ipc.h>
#include <libadt/list.h>

#define DEVMAP_NAME_MAXLEN 512

typedef enum {
	DEVMAP_DRIVER_REGISTER = IPC_FIRST_USER_METHOD,
	DEVMAP_DRIVER_UNREGISTER,
	DEVMAP_DEVICE_REGISTER,
	DEVMAP_DEVICE_UNREGISTER,
	DEVMAP_DEVICE_GET_NAME,
	DEVMAP_DEVICE_GET_HANDLE
} devmap_request_t;

/** Representation of device driver.
 * Each driver is responsible for a set of devices.
 */
typedef struct {
		/** Pointers to previous and next drivers in linked list */
	link_t drivers;	
		/** Pointer to the linked list of devices controlled by
		 * this driver */
	link_t devices;
		/** Phone asociated with this driver */
	ipcarg_t phone;
		/** Device driver name */
	char *name;	
		/** Futex for list of devices owned by this driver */
	atomic_t devices_futex;
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
	int handle;
		/** Device name */
	char *name;
		/** Device driver handling this device */
	devmap_driver_t *driver;
} devmap_device_t;

/** Interface provided by devmap. 
 * Every process that connects to devmap must ask one of following
 * interfaces otherwise connection will be refused.
 */
typedef enum {
		/** Connect as device driver */
	DEVMAP_DRIVER = 1,	
		/** Connect as client */
	DEVMAP_CLIENT,
		/** Create new connection to instance of device that
		 * is specified by second argument of call. */
	DEVMAP_CONNECT_TO_DEVICE
} devmap_interface_t;

#endif

