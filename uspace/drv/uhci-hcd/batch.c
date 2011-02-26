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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>
#include <str_error.h>

#include <usb/debug.h>

#include "batch.h"
#include "transfer_list.h"
#include "uhci.h"
#include "utils/malloc32.h"

#define DEFAULT_ERROR_COUNT 3

static int batch_schedule(batch_t *instance);

static void batch_call_in(batch_t *instance);
static void batch_call_out(batch_t *instance);
static void batch_call_in_and_dispose(batch_t *instance);
static void batch_call_out_and_dispose(batch_t *instance);


batch_t * batch_get(ddf_fun_t *fun, usb_target_t target,
    usb_transfer_type_t transfer_type, size_t max_packet_size,
    usb_speed_t speed, char *buffer, size_t size,
    char* setup_buffer, size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(func_in == NULL || func_out == NULL);
	assert(func_in != NULL || func_out != NULL);

	batch_t *instance = malloc(sizeof(batch_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate batch instance.\n");
		return NULL;
	}

	instance->qh = queue_head_get();
	if (instance->qh == NULL) {
		usb_log_error("Failed to allocate queue head.\n");
		free(instance);
		return NULL;
	}

	instance->packets = (size + max_packet_size - 1) / max_packet_size;
	if (transfer_type == USB_TRANSFER_CONTROL) {
		instance->packets += 2;
	}

	instance->tds = malloc32(sizeof(transfer_descriptor_t) * instance->packets);
	if (instance->tds == NULL) {
		usb_log_error("Failed to allocate transfer descriptors.\n");
		queue_head_dispose(instance->qh);
		free(instance);
		return NULL;
	}
	bzero(instance->tds, sizeof(transfer_descriptor_t) * instance->packets);

	const size_t transport_size = max_packet_size * instance->packets;

	instance->transport_buffer =
	   (size > 0) ? malloc32(transport_size) : NULL;
	if ((size > 0) && (instance->transport_buffer == NULL)) {
		usb_log_error("Failed to allocate device accessible buffer.\n");
		queue_head_dispose(instance->qh);
		free32(instance->tds);
		free(instance);
		return NULL;
	}

	instance->setup_buffer = setup_buffer ? malloc32(setup_size) : NULL;
	if ((setup_size > 0) && (instance->setup_buffer == NULL)) {
		usb_log_error("Failed to allocate device accessible setup buffer.\n");
		queue_head_dispose(instance->qh);
		free32(instance->tds);
		free32(instance->transport_buffer);
		free(instance);
		return NULL;
	}
	if (instance->setup_buffer) {
		memcpy(instance->setup_buffer, setup_buffer, setup_size);
	}

	instance->max_packet_size = max_packet_size;

	link_initialize(&instance->link);

	instance->target = target;
	instance->transfer_type = transfer_type;

	if (func_out)
		instance->callback_out = func_out;
	if (func_in)
		instance->callback_in = func_in;

	instance->buffer = buffer;
	instance->buffer_size = size;
	instance->setup_size = setup_size;
	instance->fun = fun;
	instance->arg = arg;
	instance->speed = speed;

	queue_head_element_td(instance->qh, addr_to_phys(instance->tds));
	return instance;
}
/*----------------------------------------------------------------------------*/
bool batch_is_complete(batch_t *instance)
{
	assert(instance);
	usb_log_debug2("Checking(%p) %d packet for completion.\n",
	    instance, instance->packets);
	instance->transfered_size = 0;
	size_t i = 0;
	for (;i < instance->packets; ++i) {
		if (transfer_descriptor_is_active(&instance->tds[i])) {
			return false;
		}
		instance->error = transfer_descriptor_status(&instance->tds[i]);
		if (instance->error != EOK) {
			if (i > 0)
				instance->transfered_size -= instance->setup_size;
			return true;
		}
		instance->transfered_size +=
		    transfer_descriptor_actual_size(&instance->tds[i]);
	}
	instance->transfered_size -= instance->setup_size;
	return true;
}
/*----------------------------------------------------------------------------*/
void batch_control_write(batch_t *instance)
{
	assert(instance);

	/* we are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer, instance->buffer_size);

	const bool low_speed = instance->speed == USB_SPEED_LOW;
	int toggle = 0;
	/* setup stage */
	transfer_descriptor_init(instance->tds, DEFAULT_ERROR_COUNT,
	    instance->setup_size, toggle, false, low_speed,
	    instance->target, USB_PID_SETUP, instance->setup_buffer,
	    &instance->tds[1]);

	/* data stage */
	size_t i = 1;
	for (;i < instance->packets - 1; ++i) {
		char *data =
		    instance->transport_buffer + ((i - 1) * instance->max_packet_size);
		toggle = 1 - toggle;

		transfer_descriptor_init(&instance->tds[i], DEFAULT_ERROR_COUNT,
		    instance->max_packet_size, toggle++, false, low_speed,
		    instance->target, USB_PID_OUT, data, &instance->tds[i + 1]);
	}

	/* status stage */
	i = instance->packets - 1;
	transfer_descriptor_init(&instance->tds[i], DEFAULT_ERROR_COUNT,
	    0, 1, false, low_speed, instance->target, USB_PID_IN, NULL, NULL);

	instance->tds[i].status |= TD_STATUS_COMPLETE_INTERRUPT_FLAG;
	usb_log_debug("Control write last TD status: %x.\n",
		instance->tds[i].status);

	instance->next_step = batch_call_out_and_dispose;
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_read(batch_t *instance)
{
	assert(instance);

	const bool low_speed = instance->speed == USB_SPEED_LOW;
	int toggle = 0;
	/* setup stage */
	transfer_descriptor_init(instance->tds, DEFAULT_ERROR_COUNT,
	    instance->setup_size, toggle, false, low_speed, instance->target,
	    USB_PID_SETUP, instance->setup_buffer, &instance->tds[1]);

	/* data stage */
	size_t i = 1;
	for (;i < instance->packets - 1; ++i) {
		char *data =
		    instance->transport_buffer + ((i - 1) * instance->max_packet_size);
		toggle = 1 - toggle;

		transfer_descriptor_init(&instance->tds[i], DEFAULT_ERROR_COUNT,
		    instance->max_packet_size, toggle, false, low_speed,
				instance->target, USB_PID_IN, data, &instance->tds[i + 1]);
	}

	/* status stage */
	i = instance->packets - 1;
	transfer_descriptor_init(&instance->tds[i], DEFAULT_ERROR_COUNT,
	    0, 1, false, low_speed, instance->target, USB_PID_OUT, NULL, NULL);

	instance->tds[i].status |= TD_STATUS_COMPLETE_INTERRUPT_FLAG;
	usb_log_debug("Control read last TD status: %x.\n",
		instance->tds[i].status);

	instance->next_step = batch_call_in_and_dispose;
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_in(batch_t *instance)
{
	assert(instance);

	const bool low_speed = instance->speed == USB_SPEED_LOW;
	int toggle = 1;
	size_t i = 0;
	for (;i < instance->packets; ++i) {
		char *data =
		    instance->transport_buffer + (i  * instance->max_packet_size);
		transfer_descriptor_t *next = (i + 1) < instance->packets ?
		    &instance->tds[i + 1] : NULL;
		toggle = 1 - toggle;

		transfer_descriptor_init(&instance->tds[i], DEFAULT_ERROR_COUNT,
		    instance->max_packet_size, toggle, false, low_speed,
		    instance->target, USB_PID_IN, data, next);
	}

	instance->tds[i - 1].status |= TD_STATUS_COMPLETE_INTERRUPT_FLAG;

	instance->next_step = batch_call_in_and_dispose;
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_out(batch_t *instance)
{
	assert(instance);

	memcpy(instance->transport_buffer, instance->buffer, instance->buffer_size);

	const bool low_speed = instance->speed == USB_SPEED_LOW;
	int toggle = 1;
	size_t i = 0;
	for (;i < instance->packets; ++i) {
		char *data =
		    instance->transport_buffer + (i  * instance->max_packet_size);
		transfer_descriptor_t *next = (i + 1) < instance->packets ?
		    &instance->tds[i + 1] : NULL;
		toggle = 1 - toggle;

		transfer_descriptor_init(&instance->tds[i], DEFAULT_ERROR_COUNT,
		    instance->max_packet_size, toggle++, false, low_speed,
		    instance->target, USB_PID_OUT, data, next);
	}

	instance->tds[i - 1].status |= TD_STATUS_COMPLETE_INTERRUPT_FLAG;

	instance->next_step = batch_call_out_and_dispose;
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_call_in(batch_t *instance)
{
	assert(instance);
	assert(instance->callback_in);

	memcpy(instance->buffer, instance->transport_buffer, instance->buffer_size);

	int err = instance->error;
	usb_log_info("Callback IN(%d): %s(%d), %zu.\n", instance->transfer_type,
	    str_error(err), err, instance->transfered_size);

	instance->callback_in(instance->fun,
	    err, instance->transfered_size,
	    instance->arg);
}
/*----------------------------------------------------------------------------*/
void batch_call_out(batch_t *instance)
{
	assert(instance);
	assert(instance->callback_out);

	int err = instance->error;
	usb_log_info("Callback OUT(%d): %d.\n", instance->transfer_type, err);
	instance->callback_out(instance->fun,
	    err, instance->arg);
}
/*----------------------------------------------------------------------------*/
void batch_call_in_and_dispose(batch_t *instance)
{
	assert(instance);
	batch_call_in(instance);
	usb_log_debug("Disposing batch: %p.\n", instance);
	free32(instance->tds);
	free32(instance->qh);
	free32(instance->setup_buffer);
	free32(instance->transport_buffer);
	free(instance);
}
/*----------------------------------------------------------------------------*/
void batch_call_out_and_dispose(batch_t *instance)
{
	assert(instance);
	batch_call_out(instance);
	usb_log_debug("Disposing batch: %p.\n", instance);
	free32(instance->tds);
	free32(instance->qh);
	free32(instance->setup_buffer);
	free32(instance->transport_buffer);
	free(instance);
}
/*----------------------------------------------------------------------------*/
int batch_schedule(batch_t *instance)
{
	assert(instance);
	uhci_t *hc = fun_to_uhci(instance->fun);
	assert(hc);
	return uhci_schedule(hc, instance);
}
/**
 * @}
 */
