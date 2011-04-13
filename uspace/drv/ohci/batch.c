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
#include "hcd_endpoint.h"
#include "utils/malloc32.h"
#include "hw_struct/endpoint_descriptor.h"
#include "hw_struct/transfer_descriptor.h"

typedef struct ohci_transfer_batch {
	ed_t *ed;
	td_t **tds;
	size_t td_count;
	size_t leave_td;
	char *device_buffer;
} ohci_transfer_batch_t;

static void ohci_transfer_batch_dispose(void *ohci_batch)
{
	ohci_transfer_batch_t *instance = ohci_batch;
	if (!instance)
		return;
	free32(instance->device_buffer);
	unsigned i = 0;
	if (instance->tds) {
		for (; i< instance->td_count; ++i) {
			if (i != instance->leave_td)
				free32(instance->tds[i]);
		}
		free(instance->tds);
	}
	free(instance);
}
/*----------------------------------------------------------------------------*/
static void batch_control(usb_transfer_batch_t *instance,
    usb_direction_t data_dir, usb_direction_t status_dir);
static void batch_data(usb_transfer_batch_t *instance);
/*----------------------------------------------------------------------------*/
usb_transfer_batch_t * batch_get(ddf_fun_t *fun, endpoint_t *ep,
    char *buffer, size_t buffer_size, char* setup_buffer, size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
#define CHECK_NULL_DISPOSE_RETURN(ptr, message...) \
        if (ptr == NULL) { \
                usb_log_error(message); \
                if (instance) { \
                        usb_transfer_batch_dispose(instance); \
                } \
                return NULL; \
        } else (void)0

	usb_transfer_batch_t *instance = malloc(sizeof(usb_transfer_batch_t));
	CHECK_NULL_DISPOSE_RETURN(instance,
	    "Failed to allocate batch instance.\n");
	usb_transfer_batch_init(instance, ep, buffer, NULL, buffer_size,
	    NULL, setup_size, func_in, func_out, arg, fun, NULL,
	    ohci_transfer_batch_dispose);

	hcd_endpoint_t *hcd_ep = hcd_endpoint_get(ep);
	assert(hcd_ep);

	ohci_transfer_batch_t *data = calloc(sizeof(ohci_transfer_batch_t), 1);
	CHECK_NULL_DISPOSE_RETURN(data, "Failed to allocate batch data.\n");
	instance->private_data = data;

	data->td_count =
	    ((buffer_size + OHCI_TD_MAX_TRANSFER - 1) / OHCI_TD_MAX_TRANSFER);
	if (ep->transfer_type == USB_TRANSFER_CONTROL) {
		data->td_count += 2;
	}

	/* we need one extra place for td that is currently assigned to hcd_ep*/
	data->tds = calloc(sizeof(td_t*), data->td_count + 1);
	CHECK_NULL_DISPOSE_RETURN(data->tds,
	    "Failed to allocate transfer descriptors.\n");

	data->tds[0] = hcd_ep->td;
	data->leave_td = 0;
	unsigned i = 1;
	for (; i <= data->td_count; ++i) {
		data->tds[i] = malloc32(sizeof(td_t));
		CHECK_NULL_DISPOSE_RETURN(data->tds[i],
		    "Failed to allocate TD %d.\n", i );
	}

	data->ed = hcd_ep->ed;

        if (setup_size + buffer_size > 0) {
		data->device_buffer = malloc32(setup_size + buffer_size);
                CHECK_NULL_DISPOSE_RETURN(data->device_buffer,
                    "Failed to allocate device accessible buffer.\n");
		instance->setup_buffer = data->device_buffer;
		instance->data_buffer = data->device_buffer + setup_size;
                memcpy(instance->setup_buffer, setup_buffer, setup_size);
        }

	return instance;
}
/*----------------------------------------------------------------------------*/
bool batch_is_complete(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	size_t tds = data->td_count;
	usb_log_debug("Batch(%p) checking %d td(s) for completion.\n",
	    instance, tds);
	usb_log_debug("ED: %x:%x:%x:%x.\n",
	    data->ed->status, data->ed->td_head, data->ed->td_tail,
	    data->ed->next);
	size_t i = 0;
	for (; i < tds; ++i) {
		assert(data->tds[i] != NULL);
		usb_log_debug("TD %d: %x:%x:%x:%x.\n", i,
		    data->tds[i]->status, data->tds[i]->cbp, data->tds[i]->next,
		    data->tds[i]->be);
		if (!td_is_finished(data->tds[i])) {
			return false;
		}
		instance->error = td_error(data->tds[i]);
		/* FIXME: calculate real transfered size */
		instance->transfered_size = instance->buffer_size;
		if (instance->error != EOK) {
			usb_log_debug("Batch(%p) found error TD(%d):%x.\n",
			    instance, i, data->tds[i]->status);
			break;
		}
	}
	data->leave_td = ++i;
	assert(data->leave_td <= data->td_count);
	return true;
}
/*----------------------------------------------------------------------------*/
void batch_commit(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	ed_set_end_td(data->ed, data->tds[data->td_count]);
}
/*----------------------------------------------------------------------------*/
void batch_control_write(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->data_buffer, instance->buffer, instance->buffer_size);
	instance->next_step = usb_transfer_batch_call_out_and_dispose;
	batch_control(instance, USB_DIRECTION_OUT, USB_DIRECTION_IN);
	usb_log_debug("Batch(%p) CONTROL WRITE initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_read(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_control(instance, USB_DIRECTION_IN, USB_DIRECTION_OUT);
	usb_log_debug("Batch(%p) CONTROL READ initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) INTERRUPT IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->data_buffer, instance->buffer, instance->buffer_size);
	instance->next_step = usb_transfer_batch_call_out_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) INTERRUPT OUT initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
ed_t * batch_ed(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	return data->ed;
}
/*----------------------------------------------------------------------------*/
void batch_control(usb_transfer_batch_t *instance,
    usb_direction_t data_dir, usb_direction_t status_dir)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	ed_init(data->ed, instance->ep);
//	ed_add_tds(data->ed, &data->tds[0], &data->tds[data->td_count - 1]);
	usb_log_debug("Created ED(%p): %x:%x:%x:%x.\n", data->ed,
	    data->ed->status, data->ed->td_tail, data->ed->td_head,
	    data->ed->next);
	int toggle = 0;
	/* setup stage */
	td_init(data->tds[0], USB_DIRECTION_BOTH, instance->setup_buffer,
		instance->setup_size, toggle);
	td_set_next(data->tds[0], data->tds[1]);
	usb_log_debug("Created SETUP TD: %x:%x:%x:%x.\n", data->tds[0]->status,
	    data->tds[0]->cbp, data->tds[0]->next, data->tds[0]->be);

	/* data stage */
	size_t td_current = 1;
	size_t remain_size = instance->buffer_size;
	char *buffer = instance->data_buffer;
	while (remain_size > 0) {
		size_t transfer_size = remain_size > OHCI_TD_MAX_TRANSFER ?
		    OHCI_TD_MAX_TRANSFER : remain_size;
		toggle = 1 - toggle;

		td_init(data->tds[td_current], data_dir, buffer,
		    transfer_size, toggle);
		td_set_next(data->tds[td_current], data->tds[td_current + 1]);
		usb_log_debug("Created DATA TD: %x:%x:%x:%x.\n",
		    data->tds[td_current]->status, data->tds[td_current]->cbp,
		    data->tds[td_current]->next, data->tds[td_current]->be);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < data->td_count - 1);
		++td_current;
	}

	/* status stage */
	assert(td_current == data->td_count - 1);
	td_init(data->tds[td_current], status_dir, NULL, 0, 1);
	td_set_next(data->tds[td_current], data->tds[td_current + 1]);
	usb_log_debug("Created STATUS TD: %x:%x:%x:%x.\n",
	    data->tds[td_current]->status, data->tds[td_current]->cbp,
	    data->tds[td_current]->next, data->tds[td_current]->be);
}
/*----------------------------------------------------------------------------*/
void batch_data(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	ed_init(data->ed, instance->ep);
//	ed_add_tds(data->ed, &data->tds[0], &data->tds[data->td_count - 1]);
	usb_log_debug("Created ED(%p): %x:%x:%x:%x.\n", data->ed,
	    data->ed->status, data->ed->td_tail, data->ed->td_head,
	    data->ed->next);

	/* data stage */
	size_t td_current = 0;
	size_t remain_size = instance->buffer_size;
	char *buffer = instance->data_buffer;
	while (remain_size > 0) {
		size_t transfer_size = remain_size > OHCI_TD_MAX_TRANSFER ?
		    OHCI_TD_MAX_TRANSFER : remain_size;

		td_init(data->tds[td_current], instance->ep->direction,
		    buffer, transfer_size, -1);
		td_set_next(data->tds[td_current], data->tds[td_current + 1]);
		usb_log_debug("Created DATA TD: %x:%x:%x:%x.\n",
		    data->tds[td_current]->status, data->tds[td_current]->cbp,
		    data->tds[td_current]->next, data->tds[td_current]->be);

		buffer += transfer_size;
		remain_size -= transfer_size;
		assert(td_current < data->td_count);
		++td_current;
	}
}
/**
 * @}
 */
