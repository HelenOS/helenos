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

/** OHCI specific data required for USB transfer */
typedef struct ohci_transfer_batch {
	/** Endpoint descriptor of the target endpoint. */
	ed_t *ed;
	/** List of TDs needed for the transfer */
	td_t **tds;
	/** Number of TDs used by the transfer */
	size_t td_count;
	/** Dummy TD to be left at the ED and used by the next transfer */
	size_t leave_td;
	/** Data buffer, must be accessible byb the OHCI hw. */
	void *device_buffer;
} ohci_transfer_batch_t;
/*----------------------------------------------------------------------------*/
static void batch_control(usb_transfer_batch_t *instance,
    usb_direction_t data_dir, usb_direction_t status_dir);
static void batch_data(usb_transfer_batch_t *instance);
/*----------------------------------------------------------------------------*/
/** Safely destructs ohci_transfer_batch_t structure
 *
 * @param[in] ohci_batch Instance to destroy.
 */
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
/** Allocate memory initialize internal structures
 *
 * @param[in] fun DDF function to pass to callback.
 * @param[in] ep Communication target
 * @param[in] buffer Data source/destination.
 * @param[in] buffer_size Size of the buffer.
 * @param[in] setup_buffer Setup data source (if not NULL)
 * @param[in] setup_size Size of setup_buffer (should be always 8)
 * @param[in] func_in function to call on inbound transfer completion
 * @param[in] func_out function to call on outbound transfer completion
 * @param[in] arg additional parameter to func_in or func_out
 * @return Valid pointer if all structures were successfully created,
 * NULL otherwise.
 *
 * Allocates and initializes structures needed by the OHCI hw for the transfer.
 */
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

	/* We need an extra place for TD that is currently assigned to hcd_ep*/
	data->tds = calloc(sizeof(td_t*), data->td_count + 1);
	CHECK_NULL_DISPOSE_RETURN(data->tds,
	    "Failed to allocate transfer descriptors.\n");

	/* Add TD left over by the previous transfer */
	data->tds[0] = hcd_ep->td;
	data->leave_td = 0;
	unsigned i = 1;
	for (; i <= data->td_count; ++i) {
		data->tds[i] = malloc32(sizeof(td_t));
		CHECK_NULL_DISPOSE_RETURN(data->tds[i],
		    "Failed to allocate TD %d.\n", i );
	}

	data->ed = hcd_ep->ed;

	/* NOTE: OHCI is capable of handling buffer that crosses page boundaries
	 * it is, however, not capable of handling buffer that accupies more
	 * than two pages (the first page is computete using start pointer, the
	 * other using end pointer)*/
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
/** Check batch TDs' status.
 *
 * @param[in] instance Batch structure to use.
 * @return False, if there is an active TD, true otherwise.
 *
 * Walk all TDs (usually there is just one). Stop with false if there is an
 * active TD. Stop with true if an error is found. Return true if the walk
 * completes with the last TD.
 */
bool batch_is_complete(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	size_t tds = data->td_count;
	usb_log_debug("Batch(%p) checking %zu td(s) for completion.\n",
	    instance, tds);
	usb_log_debug("ED: %x:%x:%x:%x.\n",
	    data->ed->status, data->ed->td_head, data->ed->td_tail,
	    data->ed->next);
	size_t i = 0;
	instance->transfered_size = instance->buffer_size;
	for (; i < tds; ++i) {
		assert(data->tds[i] != NULL);
		usb_log_debug("TD %zu: %x:%x:%x:%x.\n", i,
		    data->tds[i]->status, data->tds[i]->cbp, data->tds[i]->next,
		    data->tds[i]->be);
		if (!td_is_finished(data->tds[i])) {
			return false;
		}
		instance->error = td_error(data->tds[i]);
		if (instance->error != EOK) {
			usb_log_debug("Batch(%p) found error TD(%zu):%x.\n",
			    instance, i, data->tds[i]->status);
			/* Make sure TD queue is empty (one TD),
			 * ED should be marked as halted */
			data->ed->td_tail =
			    (data->ed->td_head & ED_TDTAIL_PTR_MASK);
			++i;
			break;
		}
	}
	data->leave_td = i;
	assert(data->leave_td <= data->td_count);
	hcd_endpoint_t *hcd_ep = hcd_endpoint_get(instance->ep);
	assert(hcd_ep);
	hcd_ep->td = data->tds[i];
	if (i > 0)
		instance->transfered_size -= td_remain_size(data->tds[i - 1]);

	/* Clear possible ED HALT */
	data->ed->td_head &= ~ED_TDHEAD_HALTED_FLAG;
	const uint32_t pa = addr_to_phys(hcd_ep->td);
	assert(pa == (data->ed->td_head & ED_TDHEAD_PTR_MASK));
	assert(pa == (data->ed->td_tail & ED_TDTAIL_PTR_MASK));

	return true;
}
/*----------------------------------------------------------------------------*/
/** Starts execution of the TD list
 *
 * @param[in] instance Batch structure to use
 */
void batch_commit(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	ed_set_end_td(data->ed, data->tds[data->td_count]);
}
/*----------------------------------------------------------------------------*/
/** Prepares control write transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Uses generic control transfer using direction OUT(data stage) and
 * IN(status stage).
 */
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
/** Prepares control read transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Uses generic control transfer using direction IN(data stage) and
 * OUT(status stage).
 */
void batch_control_read(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_control(instance, USB_DIRECTION_IN, USB_DIRECTION_OUT);
	usb_log_debug("Batch(%p) CONTROL READ initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare interrupt in transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer.
 */
void batch_interrupt_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) INTERRUPT IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare interrupt out transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer.
 */
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
/** Prepare bulk in transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer.
 */
void batch_bulk_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare bulk out transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer.
 */
void batch_bulk_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->data_buffer, instance->buffer, instance->buffer_size);
	instance->next_step = usb_transfer_batch_call_out_and_dispose;
	batch_data(instance);
	usb_log_debug("Batch(%p) BULK OUT initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare generic control transfer
 *
 * @param[in] instance Batch structure to use.
 * @param[in] data_dir Direction to use for data stage.
 * @param[in] status_dir Direction to use for status stage.
 *
 * Setup stage with toggle 0 and direction BOTH(SETUP_PID)
 * Data stage with alternating toggle and direction supplied by parameter.
 * Status stage with toggle 1 and direction supplied by parameter.
 */
void batch_control(usb_transfer_batch_t *instance,
    usb_direction_t data_dir, usb_direction_t status_dir)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	usb_log_debug("Using ED(%p): %x:%x:%x:%x.\n", data->ed,
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
/** Prepare generic data transfer
 *
 * @param[in] instance Batch structure to use.
 *
 * Direction is supplied by the associated ep and toggle is maintained by the
 * OHCI hw in ED.
 */
void batch_data(usb_transfer_batch_t *instance)
{
	assert(instance);
	ohci_transfer_batch_t *data = instance->private_data;
	assert(data);
	usb_log_debug("Using ED(%p): %x:%x:%x:%x.\n", data->ed,
	    data->ed->status, data->ed->td_tail, data->ed->td_head,
	    data->ed->next);

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
