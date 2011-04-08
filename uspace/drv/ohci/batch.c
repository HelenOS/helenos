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

static void batch_control(usb_transfer_batch_t *instance);
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
	free32(instance->transport_buffer);
	free32(instance->setup_buffer);
	free(instance);
}
/*----------------------------------------------------------------------------*/
bool batch_is_complete(usb_transfer_batch_t *instance)
{
	// TODO: implement
	return false;
}
/*----------------------------------------------------------------------------*/
void batch_control_write(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer,
	    instance->buffer_size);
	instance->next_step = batch_call_out_and_dispose;
	batch_control(instance);
	usb_log_debug("Batch(%p) CONTROL WRITE initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_read(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = batch_call_in_and_dispose;
	batch_control(instance);
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
static void batch_control(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_batch_t *data = instance->private_data;
	assert(data);
	ed_init(data->ed, instance->ep);
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
