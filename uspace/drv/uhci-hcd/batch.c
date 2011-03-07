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

#include <usb/usb.h>
#include <usb/debug.h>

#include "batch.h"
#include "transfer_list.h"
#include "uhci.h"
#include "utils/malloc32.h"

#define DEFAULT_ERROR_COUNT 3

static int batch_schedule(batch_t *instance);

static void batch_control(batch_t *instance,
    usb_packet_id data_stage, usb_packet_id status_stage);
static void batch_data(batch_t *instance, usb_packet_id pid);
static void batch_call_in(batch_t *instance);
static void batch_call_out(batch_t *instance);
static void batch_call_in_and_dispose(batch_t *instance);
static void batch_call_out_and_dispose(batch_t *instance);
static void batch_dispose(batch_t *instance);


batch_t * batch_get(ddf_fun_t *fun, usb_target_t target,
    usb_transfer_type_t transfer_type, size_t max_packet_size,
    usb_speed_t speed, char *buffer, size_t size,
    char* setup_buffer, size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg,
    device_keeper_t *manager
    )
{
	assert(func_in == NULL || func_out == NULL);
	assert(func_in != NULL || func_out != NULL);

#define CHECK_NULL_DISPOSE_RETURN(ptr, message...) \
	if (ptr == NULL) { \
		usb_log_error(message); \
		if (instance) { \
			batch_dispose(instance); \
		} \
		return NULL; \
	} else (void)0

	batch_t *instance = malloc(sizeof(batch_t));
	CHECK_NULL_DISPOSE_RETURN(instance,
	    "Failed to allocate batch instance.\n");
	bzero(instance, sizeof(batch_t));

	instance->qh = malloc32(sizeof(queue_head_t));
	CHECK_NULL_DISPOSE_RETURN(instance->qh,
	    "Failed to allocate batch queue head.\n");
	queue_head_init(instance->qh);

	instance->packets = (size + max_packet_size - 1) / max_packet_size;
	if (transfer_type == USB_TRANSFER_CONTROL) {
		instance->packets += 2;
	}

	instance->tds = malloc32(sizeof(td_t) * instance->packets);
	CHECK_NULL_DISPOSE_RETURN(
	    instance->tds, "Failed to allocate transfer descriptors.\n");
	bzero(instance->tds, sizeof(td_t) * instance->packets);

//	const size_t transport_size = max_packet_size * instance->packets;

	if (size > 0) {
		instance->transport_buffer = malloc32(size);
		CHECK_NULL_DISPOSE_RETURN(instance->transport_buffer,
		    "Failed to allocate device accessible buffer.\n");
	}

	if (setup_size > 0) {
		instance->setup_buffer = malloc32(setup_size);
		CHECK_NULL_DISPOSE_RETURN(instance->setup_buffer,
		    "Failed to allocate device accessible setup buffer.\n");
		memcpy(instance->setup_buffer, setup_buffer, setup_size);
	}


	link_initialize(&instance->link);

	instance->max_packet_size = max_packet_size;
	instance->target = target;
	instance->transfer_type = transfer_type;
	instance->buffer = buffer;
	instance->buffer_size = size;
	instance->setup_size = setup_size;
	instance->fun = fun;
	instance->arg = arg;
	instance->speed = speed;
	instance->manager = manager;

	if (func_out)
		instance->callback_out = func_out;
	if (func_in)
		instance->callback_in = func_in;

	queue_head_set_element_td(instance->qh, addr_to_phys(instance->tds));

	usb_log_debug("Batch(%p) %d:%d memory structures ready.\n",
	    instance, target.address, target.endpoint);
	return instance;
}
/*----------------------------------------------------------------------------*/
bool batch_is_complete(batch_t *instance)
{
	assert(instance);
	usb_log_debug2("Batch(%p) checking %d packet(s) for completion.\n",
	    instance, instance->packets);
	instance->transfered_size = 0;
	size_t i = 0;
	for (;i < instance->packets; ++i) {
		if (td_is_active(&instance->tds[i])) {
			return false;
		}

		instance->error = td_status(&instance->tds[i]);
		if (instance->error != EOK) {
			usb_log_debug("Batch(%p) found error TD(%d):%x.\n",
			    instance, i, instance->tds[i].status);

			device_keeper_set_toggle(instance->manager,
			    instance->target, td_toggle(&instance->tds[i]));
			if (i > 0)
				goto substract_ret;
			return true;
		}

		instance->transfered_size += td_act_size(&instance->tds[i]);
		if (td_is_short(&instance->tds[i]))
			goto substract_ret;
	}
substract_ret:
	instance->transfered_size -= instance->setup_size;
	return true;
}
/*----------------------------------------------------------------------------*/
void batch_control_write(batch_t *instance)
{
	assert(instance);
	/* we are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer, instance->buffer_size);
	batch_control(instance, USB_PID_OUT, USB_PID_IN);
	instance->next_step = batch_call_out_and_dispose;
	usb_log_debug("Batch(%p) CONTROL WRITE initialized.\n", instance);
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_read(batch_t *instance)
{
	assert(instance);
	batch_control(instance, USB_PID_IN, USB_PID_OUT);
	instance->next_step = batch_call_in_and_dispose;
	usb_log_debug("Batch(%p) CONTROL READ initialized.\n", instance);
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_in(batch_t *instance)
{
	assert(instance);
	batch_data(instance, USB_PID_IN);
	instance->next_step = batch_call_in_and_dispose;
	usb_log_debug("Batch(%p) INTERRUPT IN initialized.\n", instance);
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_out(batch_t *instance)
{
	assert(instance);
	/* we are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer, instance->buffer_size);
	batch_data(instance, USB_PID_OUT);
	instance->next_step = batch_call_out_and_dispose;
	usb_log_debug("Batch(%p) INTERRUPT OUT initialized.\n", instance);
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_in(batch_t *instance)
{
	assert(instance);
	batch_data(instance, USB_PID_IN);
	instance->next_step = batch_call_in_and_dispose;
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_out(batch_t *instance)
{
	assert(instance);
	memcpy(instance->transport_buffer, instance->buffer, instance->buffer_size);
	batch_data(instance, USB_PID_OUT);
	instance->next_step = batch_call_out_and_dispose;
	usb_log_debug("Batch(%p) BULK OUT initialized.\n", instance);
	batch_schedule(instance);
}
/*----------------------------------------------------------------------------*/
void batch_data(batch_t *instance, usb_packet_id pid)
{
	assert(instance);
	const bool low_speed = instance->speed == USB_SPEED_LOW;
	int toggle =
	    device_keeper_get_toggle(instance->manager, instance->target);
	assert(toggle == 0 || toggle == 1);

	size_t packet = 0;
	size_t remain_size = instance->buffer_size;
	while (remain_size > 0) {
		char *data =
		    instance->transport_buffer + instance->buffer_size
		    - remain_size;

		const size_t packet_size =
		    (instance->max_packet_size > remain_size) ?
		    remain_size : instance->max_packet_size;

		td_t *next_packet = (packet + 1 < instance->packets)
		    ? &instance->tds[packet + 1] : NULL;

		assert(packet < instance->packets);
		assert(packet_size <= remain_size);

		td_init(
		    &instance->tds[packet], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, instance->target, pid, data,
		    next_packet);


		toggle = 1 - toggle;
		remain_size -= packet_size;
		++packet;
	}
	device_keeper_set_toggle(instance->manager, instance->target, toggle);
}
/*----------------------------------------------------------------------------*/
void batch_control(batch_t *instance,
   usb_packet_id data_stage, usb_packet_id status_stage)
{
	assert(instance);

	const bool low_speed = instance->speed == USB_SPEED_LOW;
	int toggle = 0;
	/* setup stage */
	td_init(instance->tds, DEFAULT_ERROR_COUNT,
	    instance->setup_size, toggle, false, low_speed, instance->target,
	    USB_PID_SETUP, instance->setup_buffer, &instance->tds[1]);

	/* data stage */
	size_t packet = 1;
	size_t remain_size = instance->buffer_size;
	while (remain_size > 0) {
		char *data =
		    instance->transport_buffer + instance->buffer_size
		    - remain_size;

		toggle = 1 - toggle;

		const size_t packet_size =
		    (instance->max_packet_size > remain_size) ?
		    remain_size : instance->max_packet_size;

		td_init(
		    &instance->tds[packet], DEFAULT_ERROR_COUNT, packet_size,
		    toggle, false, low_speed, instance->target, data_stage,
		    data, &instance->tds[packet + 1]);

		++packet;
		assert(packet < instance->packets);
		assert(packet_size <= remain_size);
		remain_size -= packet_size;
	}

	/* status stage */
	assert(packet == instance->packets - 1);
	td_init(&instance->tds[packet], DEFAULT_ERROR_COUNT,
	    0, 1, false, low_speed, instance->target, status_stage, NULL, NULL);


	instance->tds[packet].status |= TD_STATUS_COMPLETE_INTERRUPT_FLAG;
	usb_log_debug2("Control last TD status: %x.\n",
	    instance->tds[packet].status);
}
/*----------------------------------------------------------------------------*/
void batch_call_in(batch_t *instance)
{
	assert(instance);
	assert(instance->callback_in);

	/* we are data in, we need data */
	memcpy(instance->buffer, instance->transport_buffer, instance->buffer_size);

	int err = instance->error;
	usb_log_debug("Batch(%p) callback IN(type:%d): %s(%d), %zu.\n",
	    instance, instance->transfer_type, str_error(err), err,
	    instance->transfered_size);

	instance->callback_in(
	    instance->fun, err, instance->transfered_size, instance->arg);
}
/*----------------------------------------------------------------------------*/
void batch_call_out(batch_t *instance)
{
	assert(instance);
	assert(instance->callback_out);

	int err = instance->error;
	usb_log_debug("Batch(%p) callback OUT(type:%d): %s(%d).\n",
	    instance, instance->transfer_type, str_error(err), err);
	instance->callback_out(instance->fun,
	    err, instance->arg);
}
/*----------------------------------------------------------------------------*/
void batch_call_in_and_dispose(batch_t *instance)
{
	assert(instance);
	batch_call_in(instance);
	batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
void batch_call_out_and_dispose(batch_t *instance)
{
	assert(instance);
	batch_call_out(instance);
	batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
void batch_dispose(batch_t *instance)
{
	assert(instance);
	usb_log_debug("Batch(%p) disposing.\n", instance);
	/* free32 is NULL safe */
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
