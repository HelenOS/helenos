/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver USB transaction structure
 */
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>
#include <usb/debug.h>

#include "batch.h"
#include "utils/malloc32.h"
#include "hw_struct/endpoint_descriptor.h"
#include "hw_struct/transfer_descriptor.h"

typedef struct ohci_batch {
	ed_t *ed;
	td_t *tds;
	size_t td_count;
} ohci_batch_t;

static void batch_control(usb_transfer_batch_t *instance,
    usb_direction_t data_dir, usb_direction_t status_dir);
static void batch_call_in_and_dispose(usb_transfer_batch_t *instance);
static void batch_call_out_and_dispose(usb_transfer_batch_t *instance);

#define DEFAULT_ERROR_COUNT 3
usb_transfer_batch_t * batch_get(ddf_fun_t *fun, endpoint_t *ep,
    char *buffer, size_t buffer_size, char* setup_buffer, size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
#define CHECK_NULL_DISPOSE_RETURN(ptr, message...) \
        if (ptr == NULL) { \
                usb_log_error(message); \
                if (instance) { \
                        batch_dispose(instance); \
                } \
                return NULL; \
        } else (void)0

	usb_transfer_batch_t *instance = malloc(sizeof(usb_transfer_batch_t));
	CHECK_NULL_DISPOSE_RETURN(instance,
	    "Failed to allocate batch instance.\n");
	usb_target_t target =
	    { .address = ep->address, .endpoint = ep->endpoint };
	usb_transfer_batch_init(instance, target, ep->transfer_type, ep->speed,
	    ep->max_packet_size, buffer, NULL, buffer_size, NULL, setup_size,
	    func_in, func_out, arg, fun, ep, NULL);

	ohci_batch_t *data = malloc(sizeof(ohci_batch_t));
	CHECK_NULL_DISPOSE_RETURN(data, "Failed to allocate batch data.\n");
	bzero(data, sizeof(ohci_batch_t));
	instance->private_data = data;

	/* we needs + 1 transfer descriptor as the last one won't be executed */
	data->td_count = 1 +
	    ((buffer_size + OHCI_TD_MAX_TRANSFER - 1) / OHCI_TD_MAX_TRANSFER);
	if (ep->transfer_type == USB_TRANSFER_CONTROL) {
		data->td_count += 2;
	}

	data->tds = malloc32(sizeof(td_t) * data->td_count);
	CHECK_NULL_DISPOSE_RETURN(data->tds,
	    "Failed to allocate transfer descriptors.\n");
	bzero(data->tds, sizeof(td_t) * data->td_count);

	data->ed = malloc32(sizeof(ed_t));
	CHECK_NULL_DISPOSE_RETURN(data->ed,
	    "Failed to allocate endpoint descriptor.\n");

        if (buffer_size > 0) {
                instance->transport_buffer = malloc32(buffer_size);
                CHECK_NULL_DISPOSE_RETURN(instance->transport_buffer,
                    "Failed to allocate device accessible buffer.\n");
        }

        if (setup_size > 0) {
                instance->setup_buffer = malloc32(setup_size);
                CHECK_NULL_DISPOSE_RETURN(instance->setup_buffer,
                    "Failed to allocate device accessible setup buffer.\n");
                memcpy(instance->setup_buffer, setup_buffer, setup_size);
        }

	return instance;
}
/*----------------------------------------------------------------------------*/
void batch_dispose(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_batch_t *data = instance->private_data;
	assert(data);
	free32(data->ed);
	free32(data->tds);
	free32(instance->setup_buffer);
	free32(instance->transport_buffer);
	free(data);
	free(instance);
}
/*----------------------------------------------------------------------------*/
bool batch_is_complete(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_batch_t *data = instance->private_data;
	assert(data);
	size_t tds = data->td_count - 1;
	usb_log_debug2("Batch(%p) checking %d td(s) for completion.\n",
	    instance, tds);
	size_t i = 0;
	for (; i < tds; ++i) {
		if (!td_is_finished(&data->tds[i]))
			return false;
		instance->error = td_error(&data->tds[i]);
		/* FIXME: calculate real transfered size */
		instance->transfered_size = instance->buffer_size;
		if (instance->error != EOK) {
			usb_log_debug("Batch(%p) found error TD(%d):%x.\n",
			    instance, i, data->tds[i].status);
			return true;
//			endpoint_toggle_set(instance->ep,
		}
	}
	return true;
}
/*----------------------------------------------------------------------------*/
void batch_control_write(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer,
	    instance->buffer_size);
	instance->next_step = batch_call_out_and_dispose;
	batch_control(instance, USB_DIRECTION_OUT, USB_DIRECTION_IN);
	usb_log_debug("Batch(%p) CONTROL WRITE initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_read(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = batch_call_in_and_dispose;
	batch_control(instance, USB_DIRECTION_IN, USB_DIRECTION_OUT);
	usb_log_debug("Batch(%p) CONTROL READ initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	assert(instance->direction == USB_DIRECTION_IN);
	instance->next_step = batch_call_in_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) INTERRUPT IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	assert(instance->direction == USB_DIRECTION_OUT);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer,
	    instance->buffer_size);
	instance->next_step = batch_call_out_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) INTERRUPT OUT initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->direction = USB_DIRECTION_IN;
	instance->next_step = batch_call_in_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->direction = USB_DIRECTION_IN;
	instance->next_step = batch_call_in_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
ed_t * batch_ed(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_batch_t *data = instance->private_data;
	assert(data);
	return data->ed;
}
/*----------------------------------------------------------------------------*/
void batch_control(usb_transfer_batch_t *instance,
    usb_direction_t data_dir, usb_direction_t status_dir)
{
	assert(instance);
	ohci_batch_t *data = instance->private_data;
	assert(data);
	ed_init(data->ed, instance->ep);
	ed_add_tds(data->ed, &data->tds[0], &data->tds[data->td_count - 1]);
	usb_log_debug("Created ED(%p): %x:%x:%x:%x.\n", data->ed,
	    data->ed->status, data->ed->td_tail, data->ed->td_head,
	    data->ed->next);
	int toggle = 0;
	/* setup stage */
	td_init(&data->tds[0], USB_DIRECTION_BOTH, instance->setup_buffer,
		instance->setup_size, toggle);
	td_set_next(&data->tds[0], &data->tds[1]);
	usb_log_debug("Created SETUP TD: %x:%x:%x:%x.\n", data->tds[0].status,
	    data->tds[0].cbp, data->tds[0].next, data->tds[0].be);

	/* data stage */
	size_t td_current = 1;
	size_t remain_size = instance->buffer_size;
	char *transfer_buffer = instance->transport_buffer;
	while (remain_size > 0) {
		size_t transfer_size = remain_size > OHCI_TD_MAX_TRANSFER ?
		    OHCI_TD_MAX_TRANSFER : remain_size;
		toggle = 1 - toggle;

		td_init(&data->tds[td_current], data_dir, transfer_buffer,
		    transfer_size, toggle);
		td_set_next(&data->tds[td_current], &data->tds[td_current + 1]);
		usb_log_debug("Created DATA TD: %x:%x:%x:%x.\n",
		    data->tds[td_current].status, data->tds[td_current].cbp,
		    data->tds[td_current].next, data->tds[td_current].be);

		transfer_buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < data->td_count - 2);
		++td_current;
	}

	/* status stage */
	assert(td_current == data->td_count - 2);
	td_init(&data->tds[td_current], status_dir, NULL, 0, 1);
	usb_log_debug("Created STATUS TD: %x:%x:%x:%x.\n",
	    data->tds[td_current].status, data->tds[td_current].cbp,
	    data->tds[td_current].next, data->tds[td_current].be);
}
/*----------------------------------------------------------------------------*/
/** Helper function calls callback and correctly disposes of batch structure.
 *
 * @param[in] instance Batch structure to use.
 */
void batch_call_in_and_dispose(usb_transfer_batch_t *instance)
{
	assert(instance);
	usb_transfer_batch_call_in(instance);
	batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
/** Helper function calls callback and correctly disposes of batch structure.
 *
 * @param[in] instance Batch structure to use.
 */
void batch_call_out_and_dispose(usb_transfer_batch_t *instance)
{
	assert(instance);
	usb_transfer_batch_call_out(instance);
	batch_dispose(instance);
}
/**
 * @}
 */
