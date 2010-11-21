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

/** @addtogroup libusbvirt usb
 * @{
 */
/** @file
 * @brief Transaction processing.
 */
#include <errno.h>
#include <stdlib.h>
#include <mem.h>

#include "hub.h"
#include "private.h"

static usb_direction_t setup_transaction_direction(usbvirt_device_t *,
    usb_endpoint_t, void *, size_t);
static void process_control_transfer(usbvirt_device_t *,
    usb_endpoint_t, usbvirt_control_transfer_t *);

/** Convert virtual USB transaction type to string.
 */
const char *usbvirt_str_transaction_type(usbvirt_transaction_type_t type)
{
	switch (type) {
		case USBVIRT_TRANSACTION_SETUP:
			return "setup";
		case USBVIRT_TRANSACTION_IN:
			return "in";
		case USBVIRT_TRANSACTION_OUT:
			return "out";
		default:
			return "unknown";
	}
}

/** SETUP transaction handling.
 * The setup transaction only prepares control transfer on given endpoint.
 */
int transaction_setup(usbvirt_device_t *device, usb_endpoint_t endpoint,
    void *buffer, size_t size)
{
	device->lib_debug(device, 1, USBVIRT_DEBUGTAG_TRANSACTION,
	    "setup transaction: endpoint=%d, size=%u", endpoint, size);
	
	usbvirt_control_transfer_t *transfer = &device->current_control_transfers[endpoint];
	
	if (transfer->request != NULL) {
		free(transfer->request);
	}
	if (transfer->data != NULL) {
		free(transfer->data);
	}
	
	transfer->direction = setup_transaction_direction(device, endpoint,
	    buffer, size);
	transfer->request = malloc(size);
	memcpy(transfer->request, buffer, size);
	transfer->request_size = size;
	transfer->data = NULL;
	transfer->data_size = 0;
	
	if (transfer->direction == USB_DIRECTION_IN) {
		process_control_transfer(device, endpoint, transfer);
	}
	
	return EOK;
}

/** OUT transaction handling.
 * The OUT transaction can trigger processing of a control transfer.
 */
int transaction_out(usbvirt_device_t *device, usb_endpoint_t endpoint,
    void *buffer, size_t size)
{
	device->lib_debug(device, 1, USBVIRT_DEBUGTAG_TRANSACTION,
	    "out transaction: endpoint=%d, size=%u", endpoint, size);
	
	/*
	 * First check whether it is a transaction over control pipe.
	 */
	usbvirt_control_transfer_t *transfer = &device->current_control_transfers[endpoint];
	if (transfer->request != NULL) {
		if (transfer->direction == USB_DIRECTION_OUT) {
			/*
			 * For out transactions, append the data to the buffer.
			 */
			uint8_t *new_buffer = (uint8_t *) malloc(transfer->data_size + size);
			if (transfer->data) {
				memcpy(new_buffer, transfer->data, transfer->data_size);
			}
			memcpy(new_buffer + transfer->data_size, buffer, size);
			
			if (transfer->data) {
				free(transfer->data);
			}
			transfer->data = new_buffer;
			transfer->data_size += size;
		} else {
			/*
			 * For in transactions, this means end of the
			 * transaction.
			 */
			free(transfer->request);
			if (transfer->data) {
				free(transfer->data);
			}
			transfer->request = NULL;
			transfer->request_size = 0;
			transfer->data = NULL;
			transfer->data_size = 0;
		}
		
		return EOK;
	}
	
	/*
	 * Otherwise, announce that some data has come.
	 */
	if (device->ops && device->ops->on_data) {
		return device->ops->on_data(device, endpoint, buffer, size);
	} else {
		return ENOTSUP;
	}
}

/** IN transaction handling.
 * The IN transaction can trigger processing of a control transfer.
 */
int transaction_in(usbvirt_device_t *device, usb_endpoint_t endpoint,
    void *buffer, size_t size, size_t *data_size)
{
	device->lib_debug(device, 1, USBVIRT_DEBUGTAG_TRANSACTION,
	    "in transaction: endpoint=%d, size=%u", endpoint, size);
	
	/*
	 * First check whether it is a transaction over control pipe.
	 */
	usbvirt_control_transfer_t *transfer = &device->current_control_transfers[endpoint];
	if (transfer->request != NULL) {
		if (transfer->direction == USB_DIRECTION_OUT) {
			/*
			 * This means end of input data.
			 */
			process_control_transfer(device, endpoint, transfer);
		} else {
			/*
			 * For in transactions, this means sending next part
			 * of the buffer.
			 */
			// FIXME: handle when the HC wants the data back
			// in more chunks
			size_t actual_size = 0;
			if (transfer->data) {
				actual_size = transfer->data_size;
			}
			if (actual_size > size) {
				actual_size = size;
			}
			if (actual_size > 0) {
				memcpy(buffer, transfer->data, actual_size);
				if (data_size) {
					*data_size = actual_size;
				}
			}
		}
		
		return EOK;
	}
	
	if (size == 0) {
		return EINVAL;
	}
	
	int rc = 1;
	
	if (device->ops && device->ops->on_data_request) {
		rc = device->ops->on_data_request(device, endpoint, buffer, size, data_size);
	}
	
	return rc;
}

/** Determine direction of control transfer.
 * First, try the user provided callback, otherwise guess, believing that
 * it uses the same format as control pipe 0.
 */
static usb_direction_t setup_transaction_direction(usbvirt_device_t *device,
    usb_endpoint_t endpoint,
    void *data, size_t size)
{
	int direction = -1;
	if (device->ops && device->ops->decide_control_transfer_direction) {
		direction = device->ops->decide_control_transfer_direction(endpoint,
		    data, size);
	}
	
	/*
	 * If the user-supplied routine have not handled the direction
	 * (or simply was not provided) we will guess, hoping that it 
	 * uses same format as standard request on control pipe zero.
	 */
	if (direction < 0) {
		if (size > 0) {
			uint8_t *ptr = (uint8_t *) data;
			if ((ptr[0] & 128) == 128) {
				direction = USB_DIRECTION_IN;
			} else {
				direction = USB_DIRECTION_OUT;
			}
		} else {
			/* This shall not happen anyway. */
			direction = USB_DIRECTION_OUT;
		}
	}
	
	return (usb_direction_t) direction;
}

/** Process control transfer.
 */
static void process_control_transfer(usbvirt_device_t *device,
    usb_endpoint_t endpoint,
    usbvirt_control_transfer_t *transfer)
{
	int rc = EFORWARD;
	
	if (device->ops && device->ops->on_control_transfer) {
		rc = device->ops->on_control_transfer(device, endpoint, transfer);
	}
	
	if (rc == EFORWARD) {
		if (endpoint == 0) {
			rc = control_pipe(device, transfer);
		}
	}
}

/**
 * @}
 */
