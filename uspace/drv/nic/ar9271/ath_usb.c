/*
 * Copyright (c) 2015 Jan Kolarik
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @file ath_usb.c
 *
 * Implementation of Atheros USB wifi device functions.
 *
 */

#include <usb/dev/pipes.h>
#include <usb/debug.h>
#include <stdlib.h>
#include <errno.h>
#include "ath_usb.h"

static errno_t ath_usb_send_ctrl_message(ath_t *, void *, size_t);
static errno_t ath_usb_read_ctrl_message(ath_t *, void *, size_t, size_t *);
static errno_t ath_usb_send_data_message(ath_t *, void *, size_t);
static errno_t ath_usb_read_data_message(ath_t *, void *, size_t, size_t *);

static ath_ops_t ath_usb_ops = {
	.send_ctrl_message = ath_usb_send_ctrl_message,
	.read_ctrl_message = ath_usb_read_ctrl_message,
	.send_data_message = ath_usb_send_data_message,
	.read_data_message = ath_usb_read_data_message
};

/** Initialize Atheros WiFi USB device.
 *
 * @param ath Generic Atheros WiFi device structure.
 * @param usb_device  Connected USB device.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ath_usb_init(ath_t *ath, usb_device_t *usb_device, const usb_endpoint_description_t **endpoints)
{
	ath_usb_t *ath_usb = malloc(sizeof(ath_usb_t));
	if (!ath_usb) {
		usb_log_error("Failed to allocate memory for ath usb device "
		    "structure.\n");
		return ENOMEM;
	}

	ath_usb->usb_device = usb_device;

	int rc;

#define _MAP_EP(target, ep_no) do {\
	usb_endpoint_mapping_t *epm = usb_device_get_mapped_ep_desc(usb_device, endpoints[ep_no]);\
	if (!epm || !epm->present) {\
		usb_log_error("Failed to map endpoint: " #ep_no ".");\
		rc = ENOENT;\
		goto err_ath_usb;\
	}\
	target = &epm->pipe;\
	} while (0);

	_MAP_EP(ath_usb->output_data_pipe, 0);
	_MAP_EP(ath_usb->input_data_pipe, 1);
	_MAP_EP(ath_usb->input_ctrl_pipe, 2);
	_MAP_EP(ath_usb->output_ctrl_pipe, 3);

#undef _MAP_EP

	ath->ctrl_response_length = 64;
	ath->data_response_length = 512;

	ath->specific_data = ath_usb;
	ath->ops = &ath_usb_ops;

	return EOK;
err_ath_usb:
	free(ath_usb);
	return rc;
}

/** Send control message.
 *
 * @param ath         Generic Atheros WiFi device structure.
 * @param buffer      Buffer with data to send.
 * @param buffer_size Buffer size.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ath_usb_send_ctrl_message(ath_t *ath, void *buffer,
    size_t buffer_size)
{
	ath_usb_t *ath_usb = (ath_usb_t *) ath->specific_data;
	return usb_pipe_write(ath_usb->output_ctrl_pipe, buffer, buffer_size);
}

/** Read control message.
 *
 * @param ath              Generic Atheros WiFi device structure.
 * @param buffer           Buffer with data to send.
 * @param buffer_size      Buffer size.
 * @param transferred_size Real size of read data.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ath_usb_read_ctrl_message(ath_t *ath, void *buffer,
    size_t buffer_size, size_t *transferred_size)
{
	ath_usb_t *ath_usb = (ath_usb_t *) ath->specific_data;
	return usb_pipe_read(ath_usb->input_ctrl_pipe, buffer, buffer_size, transferred_size);
}

/** Send data message.
 *
 * @param ath         Generic Atheros WiFi device structure.
 * @param buffer      Buffer with data to send.
 * @param buffer_size Buffer size.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ath_usb_send_data_message(ath_t *ath, void *buffer,
    size_t buffer_size)
{
	size_t complete_buffer_size = buffer_size +
	    sizeof(ath_usb_data_header_t);
	void *complete_buffer = malloc(complete_buffer_size);
	memcpy(complete_buffer + sizeof(ath_usb_data_header_t),
	    buffer, buffer_size);

	ath_usb_data_header_t *data_header =
	    (ath_usb_data_header_t *) complete_buffer;
	data_header->length = host2uint16_t_le(buffer_size);
	data_header->tag = host2uint16_t_le(TX_TAG);

	ath_usb_t *ath_usb = (ath_usb_t *) ath->specific_data;
	const errno_t ret_val = usb_pipe_write(ath_usb->output_data_pipe,
	    complete_buffer, complete_buffer_size);

	free(complete_buffer);

	return ret_val;
}

/** Read data message.
 *
 * @param ath              Generic Atheros WiFi device structure.
 * @param buffer           Buffer with data to send.
 * @param buffer_size      Buffer size.
 * @param transferred_size Real size of read data.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ath_usb_read_data_message(ath_t *ath, void *buffer,
    size_t buffer_size, size_t *transferred_size)
{
	ath_usb_t *ath_usb = (ath_usb_t *) ath->specific_data;
	return usb_pipe_read(ath_usb->input_data_pipe, buffer, buffer_size, transferred_size);
}
