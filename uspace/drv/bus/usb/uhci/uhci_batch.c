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
/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver USB transfer structure
 */
#include <errno.h>
#include <str_error.h>

#include <usb/usb.h>
#include <usb/debug.h>

#include "uhci_batch.h"
#include "transfer_list.h"
#include "hw_struct/transfer_descriptor.h"
#include "utils/malloc32.h"

#define DEFAULT_ERROR_COUNT 3
static void batch_control_write(usb_transfer_batch_t *instance);
static void batch_control_read(usb_transfer_batch_t *instance);

static void batch_interrupt_in(usb_transfer_batch_t *instance);
static void batch_interrupt_out(usb_transfer_batch_t *instance);

static void batch_bulk_in(usb_transfer_batch_t *instance);
static void batch_bulk_out(usb_transfer_batch_t *instance);

static void batch_setup_control(usb_transfer_batch_t *batch)
{
	// TODO Find a better way to do this
	if (batch->setup_buffer[0] & (1 << 7))
		batch_control_read(batch);
	else
		batch_control_write(batch);
}

void (*batch_setup[4][3])(usb_transfer_batch_t*) =
{
	{ NULL, NULL, batch_setup_control },
	{ NULL, NULL, NULL },
	{ batch_bulk_in, batch_bulk_out, NULL },
	{ batch_interrupt_in, batch_interrupt_out, NULL },
};
 // */
/** UHCI specific data required for USB transfer */
typedef struct uhci_transfer_batch {
	/** Queue head
	 * This QH is used to maintain UHCI schedule structure and the element
	 * pointer points to the first TD of this batch.
	 */
	qh_t *qh;
	/** List of TDs needed for the transfer */
	td_t *tds;
	/** Number of TDs used by the transfer */
	size_t td_count;
	/** Data buffer, must be accessible by the UHCI hw */
	void *device_buffer;
} uhci_transfer_batch_t;
/*----------------------------------------------------------------------------*/
static void batch_control(usb_transfer_batch_t *instance,
    usb_packet_id data_stage, usb_packet_id status_stage);
static void batch_data(usb_transfer_batch_t *instance, usb_packet_id pid);
/*----------------------------------------------------------------------------*/
/** Safely destructs uhci_transfer_batch_t structure
 *
 * @param[in] uhci_batch Instance to destroy.
 */
static void uhci_transfer_batch_dispose(void *uhci_batch)
{
	uhci_transfer_batch_t *instance = uhci_batch;
	assert(instance);
	free32(instance->device_buffer);
	free(instance);
}
/*----------------------------------------------------------------------------*/
/** Allocate memory and initialize internal data structure.
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
 * Determines the number of needed transfer descriptors (TDs).
 * Prepares a transport buffer (that is accessible by the hardware).
 * Initializes parameters needed for the transfer and callback.
 */
int batch_init_uhci(usb_transfer_batch_t *batch)
{
#define CHECK_NULL_DISPOSE_RETURN(ptr, message...) \
	if (ptr == NULL) { \
		usb_log_error(message); \
		if (uhci_data) { \
			uhci_transfer_batch_dispose(uhci_data); \
		} \
		return ENOMEM; \
	} else (void)0

	uhci_transfer_batch_t *uhci_data =
	    calloc(1, sizeof(uhci_transfer_batch_t));
	CHECK_NULL_DISPOSE_RETURN(uhci_data,
	    "Failed to allocate UHCI batch.\n");

	uhci_data->td_count =
	    (batch->buffer_size + batch->ep->max_packet_size - 1)
	    / batch->ep->max_packet_size;
	if (batch->ep->transfer_type == USB_TRANSFER_CONTROL) {
		uhci_data->td_count += 2;
	}

	assert((sizeof(td_t) % 16) == 0);
	const size_t total_size = (sizeof(td_t) * uhci_data->td_count)
	    + sizeof(qh_t) + batch->setup_size + batch->buffer_size;
	uhci_data->device_buffer = malloc32(total_size);
	CHECK_NULL_DISPOSE_RETURN(uhci_data->device_buffer,
	    "Failed to allocate UHCI buffer.\n");
	bzero(uhci_data->device_buffer, total_size);

	uhci_data->tds = uhci_data->device_buffer;
	uhci_data->qh =
	    (uhci_data->device_buffer + (sizeof(td_t) * uhci_data->td_count));

	qh_init(uhci_data->qh);
	qh_set_element_td(uhci_data->qh, uhci_data->tds);

	void *setup =
	    uhci_data->device_buffer + (sizeof(td_t) * uhci_data->td_count)
	    + sizeof(qh_t);
	/* Copy SETUP packet data to device buffer */
	memcpy(setup, batch->setup_buffer, batch->setup_size);
	/* Set generic data buffer pointer */
	batch->data_buffer = setup + batch->setup_size;
	batch->private_data_dtor = uhci_transfer_batch_dispose;
	batch->private_data = uhci_data;
	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT
	    " memory structures ready.\n", batch,
	    USB_TRANSFER_BATCH_ARGS(*batch));
	assert(batch_setup[batch->ep->transfer_type][batch->ep->direction]);
	batch_setup[batch->ep->transfer_type][batch->ep->direction](batch);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Check batch TDs for activity.
 *
 * @param[in] instance Batch structure to use.
 * @return False, if there is an active TD, true otherwise.
 *
 * Walk all TDs. Stop with false if there is an active one (it is to be
 * processed). Stop with true if an error is found. Return true if the last TD
 * is reached.
 */
bool batch_is_complete(usb_transfer_batch_t *instance)
{
	assert(instance);
	uhci_transfer_batch_t *data = instance->private_data;
	assert(data);

	usb_log_debug2("Batch(%p) checking %zu transfer(s) for completion.\n",
	    instance, data->td_count);
	instance->transfered_size = 0;
	size_t i = 0;
	for (;i < data->td_count; ++i) {
		if (td_is_active(&data->tds[i])) {
			return false;
		}

		instance->error = td_status(&data->tds[i]);
		if (instance->error != EOK) {
			usb_log_debug("Batch(%p) found error TD(%zu):%"
			    PRIx32 ".\n", instance, i, data->tds[i].status);
			td_print_status(&data->tds[i]);

			assert(instance->ep != NULL);
			endpoint_toggle_set(instance->ep,
			    td_toggle(&data->tds[i]));
			if (i > 0)
				goto substract_ret;
			return true;
		}

		instance->transfered_size += td_act_size(&data->tds[i]);
		if (td_is_short(&data->tds[i]))
			goto substract_ret;
	}
substract_ret:
	instance->transfered_size -= instance->setup_size;
	return true;
}

#define LOG_BATCH_INITIALIZED(batch, name) \
	usb_log_debug2("Batch %p %s " USB_TRANSFER_BATCH_FMT " initialized.\n", \
	    (batch), (name), USB_TRANSFER_BATCH_ARGS(*(batch)))

/*----------------------------------------------------------------------------*/
/** Prepares control write transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Uses generic control function with pids OUT and IN.
 */
static void batch_control_write(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->data_buffer, instance->buffer, instance->buffer_size);
	batch_control(instance, USB_PID_OUT, USB_PID_IN);
	instance->next_step = usb_transfer_batch_call_out_and_dispose;
	LOG_BATCH_INITIALIZED(instance, "control write");
}
/*----------------------------------------------------------------------------*/
/** Prepares control read transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Uses generic control with pids IN and OUT.
 */
static void batch_control_read(usb_transfer_batch_t *instance)
{
	assert(instance);
	batch_control(instance, USB_PID_IN, USB_PID_OUT);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	LOG_BATCH_INITIALIZED(instance, "control read");
}
/*----------------------------------------------------------------------------*/
/** Prepare interrupt in transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer with PID_IN.
 */
static void batch_interrupt_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	batch_data(instance, USB_PID_IN);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	LOG_BATCH_INITIALIZED(instance, "interrupt in");
}
/*----------------------------------------------------------------------------*/
/** Prepare interrupt out transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer with PID_OUT.
 */
static void batch_interrupt_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->data_buffer, instance->buffer, instance->buffer_size);
	batch_data(instance, USB_PID_OUT);
	instance->next_step = usb_transfer_batch_call_out_and_dispose;
	LOG_BATCH_INITIALIZED(instance, "interrupt out");
}
/*----------------------------------------------------------------------------*/
/** Prepare bulk in transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer with PID_IN.
 */
static void batch_bulk_in(usb_transfer_batch_t *instance)
{
	assert(instance);
	batch_data(instance, USB_PID_IN);
	instance->next_step = usb_transfer_batch_call_in_and_dispose;
	LOG_BATCH_INITIALIZED(instance, "bulk in");
}
/*----------------------------------------------------------------------------*/
/** Prepare bulk out transfer.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transfer with PID_OUT.
 */
static void batch_bulk_out(usb_transfer_batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->data_buffer, instance->buffer, instance->buffer_size);
	batch_data(instance, USB_PID_OUT);
	instance->next_step = usb_transfer_batch_call_out_and_dispose;
	LOG_BATCH_INITIALIZED(instance, "bulk out");
}
/*----------------------------------------------------------------------------*/
/** Prepare generic data transfer
 *
 * @param[in] instance Batch structure to use.
 * @param[in] pid Pid to use for data transactions.
 *
 * Transactions with alternating toggle bit and supplied pid value.
 * The last transfer is marked with IOC flag.
 */
static void batch_data(usb_transfer_batch_t *instance, usb_packet_id pid)
{
	assert(instance);
	uhci_transfer_batch_t *data = instance->private_data;
	assert(data);

	const bool low_speed = instance->ep->speed == USB_SPEED_LOW;
	int toggle = endpoint_toggle_get(instance->ep);
	assert(toggle == 0 || toggle == 1);

	size_t td = 0;
	size_t remain_size = instance->buffer_size;
	char *buffer = instance->data_buffer;
	while (remain_size > 0) {
		const size_t packet_size =
		    (instance->ep->max_packet_size > remain_size) ?
		    remain_size : instance->ep->max_packet_size;

		td_t *next_td = (td + 1 < data->td_count)
		    ? &data->tds[td + 1] : NULL;


		usb_target_t target =
		    { instance->ep->address, instance->ep->endpoint };

		assert(td < data->td_count);
		td_init(
		    &data->tds[td], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, target, pid, buffer, next_td);

		++td;
		toggle = 1 - toggle;
		buffer += packet_size;
		assert(packet_size <= remain_size);
		remain_size -= packet_size;
	}
	td_set_ioc(&data->tds[td - 1]);
	endpoint_toggle_set(instance->ep, toggle);
}
/*----------------------------------------------------------------------------*/
/** Prepare generic control transfer
 *
 * @param[in] instance Batch structure to use.
 * @param[in] data_stage Pid to use for data tds.
 * @param[in] status_stage Pid to use for data tds.
 *
 * Setup stage with toggle 0 and USB_PID_SETUP.
 * Data stage with alternating toggle and pid supplied by parameter.
 * Status stage with toggle 1 and pid supplied by parameter.
 * The last transfer is marked with IOC.
 */
static void batch_control(usb_transfer_batch_t *instance,
   usb_packet_id data_stage, usb_packet_id status_stage)
{
	assert(instance);
	uhci_transfer_batch_t *data = instance->private_data;
	assert(data);
	assert(data->td_count >= 2);

	const bool low_speed = instance->ep->speed == USB_SPEED_LOW;
	const usb_target_t target =
	    { instance->ep->address, instance->ep->endpoint };

	/* setup stage */
	td_init(
	    data->tds, DEFAULT_ERROR_COUNT, instance->setup_size, 0, false,
	    low_speed, target, USB_PID_SETUP, instance->setup_buffer,
	    &data->tds[1]);

	/* data stage */
	size_t td = 1;
	unsigned toggle = 1;
	size_t remain_size = instance->buffer_size;
	char *buffer = instance->data_buffer;
	while (remain_size > 0) {
		const size_t packet_size =
		    (instance->ep->max_packet_size > remain_size) ?
		    remain_size : instance->ep->max_packet_size;

		td_init(
		    &data->tds[td], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, target, data_stage,
		    buffer, &data->tds[td + 1]);

		++td;
		toggle = 1 - toggle;
		buffer += packet_size;
		assert(td < data->td_count);
		assert(packet_size <= remain_size);
		remain_size -= packet_size;
	}

	/* status stage */
	assert(td == data->td_count - 1);

	td_init(
	    &data->tds[td], DEFAULT_ERROR_COUNT, 0, 1, false, low_speed,
	    target, status_stage, NULL, NULL);
	td_set_ioc(&data->tds[td]);

	usb_log_debug2("Control last TD status: %x.\n",
	    data->tds[td].status);
}
/*----------------------------------------------------------------------------*/
/** Provides access to QH data structure.
 *
 * @param[in] instance Batch pointer to use.
 * @return Pointer to the QH used by the batch.
 */
qh_t * batch_qh(usb_transfer_batch_t *instance)
{
	assert(instance);
	uhci_transfer_batch_t *data = instance->private_data;
	assert(data);
	return data->qh;
}
/**
 * @}
 */
