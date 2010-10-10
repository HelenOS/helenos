/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Virtual device management (implementation).
 */

#include <ipc/ipc.h>
#include <adt/list.h>
#include <bool.h>
#include <async.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>

#include <usbvirt/ids.h>

#include "devices.h"

#define list_foreach(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)

LIST_INITIALIZE(devices);

/** Recognise device by id.
 *
 * @param id Device id.
 * @param phone Callback phone.
 */
virtdev_connection_t *virtdev_recognise(int id, int phone)
{
	virtdev_connection_t * dev = NULL;
	switch (id) {
		case USBVIRT_DEV_KEYBOARD_ID:
			dev = virtdev_add_device(
			    USBVIRT_DEV_KEYBOARD_ADDRESS, phone);
			break;
		default:
			break;
	}
	
	/*
	 * We do not want to mess-up the virtdev_add_device() as
	 * the id is needed only before device probing/detection
	 * is implemented.
	 *
	 * However, that does not mean that this will happen soon.
	 */
	if (dev) {
		dev->id = id;
	}
	
	return dev;
}

/** Find virtual device by its USB address.
 *
 * @retval NULL No virtual device at given address.
 */
virtdev_connection_t *virtdev_find_by_address(usb_address_t address)
{
	link_t *pos;
	list_foreach(pos, &devices) {
		virtdev_connection_t *dev
		    = list_get_instance(pos, virtdev_connection_t, link);
		if (dev->address == address) {
			return dev;
		}
	}
	
	return NULL;
}

/** Create virtual device.
 *
 * @param address USB address.
 * @param phone Callback phone.
 * @return New device.
 * @retval NULL Out of memory or address already occupied.
 */
virtdev_connection_t *virtdev_add_device(usb_address_t address, int phone)
{
	link_t *pos;
	list_foreach(pos, &devices) {
		virtdev_connection_t *dev
		    = list_get_instance(pos, virtdev_connection_t, link);
		if (dev->address == address) {
			return NULL;
		}
	}
	
	virtdev_connection_t *dev = (virtdev_connection_t *)
	    malloc(sizeof(virtdev_connection_t));
	dev->phone = phone;
	dev->address = address;
	list_append(&dev->link, &devices);
	
	return dev;
}

/** Destroy virtual device.
 */
void virtdev_destroy_device(virtdev_connection_t *dev)
{
	list_remove(&dev->link);
	free(dev);
}

/**
 * @}
 */
