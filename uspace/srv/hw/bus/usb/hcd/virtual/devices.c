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
#include <usbvirt/hub.h>

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
			dev = virtdev_add_device(phone);
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

/** Create virtual device.
 *
 * @param address USB address.
 * @param phone Callback phone.
 * @return New device.
 * @retval NULL Out of memory or address already occupied.
 */
virtdev_connection_t *virtdev_add_device(int phone)
{
	virtdev_connection_t *dev = (virtdev_connection_t *)
	    malloc(sizeof(virtdev_connection_t));
	dev->phone = phone;
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

/** Send data to all connected devices.
 *
 * @param transaction Transaction to be sent over the bus.
 */
usb_transaction_outcome_t virtdev_send_to_all(transaction_t *transaction)
{
	link_t *pos;
	list_foreach(pos, &devices) {
		virtdev_connection_t *dev
		    = list_get_instance(pos, virtdev_connection_t, link);
		
		ipc_call_t answer_data;
		ipcarg_t answer_rc;
		aid_t req;
		int rc;
		
		req = async_send_3(dev->phone,
		    IPC_M_USBVIRT_DATA_TO_DEVICE,
		    transaction->target.address,
		    transaction->target.endpoint,
		    transaction->type,
		    &answer_data);
		
		rc = async_data_write_start(dev->phone,
		    transaction->buffer, transaction->len);
		if (rc != EOK) {
			async_wait_for(req, NULL);
		} else {
			async_wait_for(req, &answer_rc);
			rc = (int)answer_rc;
		}
	}
	
	/*
	 * TODO: maybe screw some transactions to get more
	 * real-life image.
	 */
	return USB_OUTCOME_OK;
}

/**
 * @}
 */
