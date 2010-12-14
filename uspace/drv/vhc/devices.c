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

#include <usbvirt/hub.h>

#include "devices.h"
#include "hub.h"
#include "vhcd.h"

#define list_foreach(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)

static LIST_INITIALIZE(devices);

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
	
	hub_add_device(dev);
	
	return dev;
}

/** Destroy virtual device.
 */
void virtdev_destroy_device(virtdev_connection_t *dev)
{
	hub_remove_device(dev);
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
		
		if (!hub_can_device_signal(dev)) {
			continue;
		}
		
		ipc_call_t answer_data;
		sysarg_t answer_rc;
		aid_t req;
		int rc = EOK;
		int method = IPC_M_USBVIRT_TRANSACTION_SETUP;
		
		switch (transaction->type) {
			case USBVIRT_TRANSACTION_SETUP:
				method = IPC_M_USBVIRT_TRANSACTION_SETUP;
				break;
			case USBVIRT_TRANSACTION_IN:
				method = IPC_M_USBVIRT_TRANSACTION_IN;
				break;
			case USBVIRT_TRANSACTION_OUT:
				method = IPC_M_USBVIRT_TRANSACTION_OUT;
				break;
		}
		
		req = async_send_3(dev->phone,
		    method,
		    transaction->target.address,
		    transaction->target.endpoint,
		    transaction->len,
		    &answer_data);
		
		if (transaction->len > 0) {
			if (transaction->type == USBVIRT_TRANSACTION_IN) {
				rc = async_data_read_start(dev->phone,
				    transaction->buffer, transaction->len);
			} else {
				rc = async_data_write_start(dev->phone,
				    transaction->buffer, transaction->len);
			}
		}
		
		if (rc != EOK) {
			async_wait_for(req, NULL);
		} else {
			async_wait_for(req, &answer_rc);
			rc = (int)answer_rc;
		}
	}
	
	/*
	 * Send the data to the virtual hub as well
	 * (if the address matches).
	 */
	if (virthub_dev.address == transaction->target.address) {
		size_t tmp;
		dprintf(1, "sending `%s' transaction to hub",
		    usbvirt_str_transaction_type(transaction->type));
		switch (transaction->type) {
			case USBVIRT_TRANSACTION_SETUP:
				virthub_dev.transaction_setup(&virthub_dev,
				    transaction->target.endpoint,
				    transaction->buffer, transaction->len);
				break;
				
			case USBVIRT_TRANSACTION_IN:
				virthub_dev.transaction_in(&virthub_dev,
				    transaction->target.endpoint,
				    transaction->buffer, transaction->len,
				    &tmp);
				if (tmp < transaction->len) {
					transaction->len = tmp;
				}
				break;
				
			case USBVIRT_TRANSACTION_OUT:
				virthub_dev.transaction_out(&virthub_dev,
				    transaction->target.endpoint,
				    transaction->buffer, transaction->len);
				break;
		}
		dprintf(4, "transaction on hub processed...");
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
