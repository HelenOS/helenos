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
#include <mem.h>

#include <usb/debug.h>

#include "tracker.h"

#define SETUP_PACKET_DATA_SIZE 8
#define DEFAULT_ERROR_COUNT 3
#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a,b) ((a < b) ? a : b)

static void tracker_control_read_data(tracker_t *instance);
static void tracker_control_write_data(tracker_t *instance);
static void tracker_control_read_status(tracker_t *instance);
static void tracker_control_write_status(tracker_t *instance);


int tracker_init(tracker_t *instance, device_t *dev, usb_target_t target,
    size_t max_packet_size, dev_speed_t speed, char *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(instance);
	assert(func_in == NULL || func_out == NULL);
	assert(func_in != NULL || func_out != NULL);

	instance->td = malloc32(sizeof(transfer_descriptor_t));
	if (!instance->td) {
		usb_log_error("Failed to allocate transfer descriptor.\n");
		return ENOMEM;
	}

	instance->packet = max_packet_size ? malloc32(max_packet_size) : NULL;
	if (max_packet_size && !instance->packet) {
		usb_log_error("Failed to allocate device acessible buffer.\n");
		free32(instance->td);
		return ENOMEM;
	}
	instance->max_packet_size = max_packet_size;
	instance->packet_size = 0;
	instance->buffer_offset = 0;

	link_initialize(&instance->link);
	instance->target = target;

	if (func_out)
		instance->callback_out = func_out;
	if (func_in)
		instance->callback_in = func_in;
	instance->buffer = buffer;
	instance->buffer_size = size;
	instance->dev = dev;
	instance->arg = arg;
	instance->toggle = 0;
	return EOK;
}
/*----------------------------------------------------------------------------*/
void tracker_control_write(tracker_t *instance)
{
	assert(instance);
	assert(instance->buffer_offset == 0);
	instance->packet_size = SETUP_PACKET_DATA_SIZE;
	memcpy(instance->packet, instance->buffer, SETUP_PACKET_DATA_SIZE);
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    SETUP_PACKET_DATA_SIZE, false, instance->target, USB_PID_SETUP,
	    instance->packet);
	instance->buffer_offset += SETUP_PACKET_DATA_SIZE;
	instance->next_step = tracker_control_write_data;
	//TODO: add to scheduler
}
/*----------------------------------------------------------------------------*/
void tracker_control_read(tracker_t *instance)
{
	assert(instance);
	assert(instance->buffer_offset == 0);
	instance->packet_size = SETUP_PACKET_DATA_SIZE;
	memcpy(instance->packet, instance->buffer, SETUP_PACKET_DATA_SIZE);
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    SETUP_PACKET_DATA_SIZE, false, instance->target, USB_PID_SETUP,
	    instance->packet);
	instance->buffer_offset += SETUP_PACKET_DATA_SIZE;
	instance->next_step = tracker_control_read_data;
	//TODO: add to scheduler
}
/*----------------------------------------------------------------------------*/
void tracker_control_read_data(tracker_t *instance)
{
	assert(instance);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	if (err != EOK) {
		tracker_call_in_and_dispose(instance);
		return;
	}

	/* we are data in, we want data from our device */
	memcpy(instance->buffer + instance->buffer_offset, instance->packet,
	    instance->packet_size);
	instance->buffer_offset += instance->packet_size;

	/* prepare next packet, no copy, we are receiving data */
	instance->packet_size =	MIN(instance->max_packet_size,
			instance->buffer_size - instance->buffer_offset);
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
			instance->packet_size, false, instance->target, USB_PID_IN,
			instance->packet);
	// TODO: add to uhci scheduler

	/* set next step */
	if ((instance->buffer_offset + instance->packet_size)
	    < instance->buffer_size) {
		/* that's all, end coomunication */
		instance->next_step = tracker_control_read_status;
	}
}
/*----------------------------------------------------------------------------*/
void tracker_control_write_data(tracker_t *instance)
{
	assert(instance);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	if (err != EOK) {
		tracker_call_out_and_dispose(instance);
		return;
	}

	/* we are data out, we down't want data from our device */
	instance->buffer_offset += instance->packet_size;

	/* prepare next packet, copy data to packet */
	instance->packet_size =	MIN(instance->max_packet_size,
			instance->buffer_size - instance->buffer_offset);
	memcpy(instance->packet, instance->buffer + instance->buffer_offset,
	    instance->packet_size);
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
			instance->packet_size, false, instance->target, USB_PID_OUT,
			instance->packet);
	// TODO: add to uhci scheduler

	/* set next step */
	if ((instance->buffer_offset + instance->packet_size)
	    < instance->buffer_size) {
		/* that's all, end coomunication */
		instance->next_step = tracker_control_write_status;
	}
}
/*----------------------------------------------------------------------------*/
void tracker_control_read_status(tracker_t *instance)
{
	assert(instance);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	if (err != EOK) {
		tracker_call_in_and_dispose(instance);
		return;
	}

	/* we are data in, we want data from our device */
	memcpy(instance->buffer + instance->buffer_offset, instance->packet,
	    instance->packet_size);
	instance->buffer_offset += instance->packet_size;
	assert(instance->buffer_offset = instance->buffer_size);

	/* prepare next packet, no nothing, just an empty packet */
	instance->packet_size =	0;
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
			0, false, instance->target, USB_PID_OUT, NULL);
	// TODO: add to uhci scheduler

	/* set next step, callback and cleanup */
	instance->next_step = tracker_call_in_and_dispose;
}
/*----------------------------------------------------------------------------*/
void tracker_control_write_status(tracker_t *instance)
{
	assert(instance);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	if (err != EOK) {
		tracker_call_out_and_dispose(instance);
		return;
	}

	/* we are data in, we want data from our device */
	memcpy(instance->buffer + instance->buffer_offset, instance->packet,
	    instance->packet_size);
	instance->buffer_offset += instance->packet_size;
	assert(instance->buffer_offset = instance->buffer_size);

	/* prepare next packet, no nothing, just an empty packet */
	instance->packet_size =	0;
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
			0, false, instance->target, USB_PID_OUT, NULL);
	// TODO: add to uhci scheduler

	/* set next step, callback and cleanup */
	instance->next_step = tracker_call_out_and_dispose;
}
/*----------------------------------------------------------------------------*/
void tracker_interrupt_in(tracker_t *instance)
{
}
/*----------------------------------------------------------------------------*/
void tracker_interrupt_out(tracker_t *instance)
{
}
/*----------------------------------------------------------------------------*/
void tracker_call_in(tracker_t *instance)
{
	assert(instance);
	assert(instance->callback_in);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	if (err == EOK && instance->packet_size) {
		memcpy(instance->buffer + instance->buffer_offset, instance->packet,
		    instance->packet_size);
	}
	instance->callback_in(instance->dev,
	    err ? USB_OUTCOME_CRCERROR : USB_OUTCOME_OK, instance->buffer_offset,
	    instance->arg);
}
/*----------------------------------------------------------------------------*/
void tracker_call_out(tracker_t *instance)
{
	assert(instance);
	assert(instance->callback_out);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	instance->callback_out(instance->dev,
	    err ? USB_OUTCOME_CRCERROR : USB_OUTCOME_OK, instance->arg);
}
/*----------------------------------------------------------------------------*/
void tracker_call_in_and_dispose(tracker_t *instance)
{
	assert(instance);
	tracker_call_in(instance);
	free32(instance->td);
	free32(instance->packet);
	free(instance);
}
/*----------------------------------------------------------------------------*/
void tracker_call_out_and_dispose(tracker_t *instance)
{
	assert(instance);
	tracker_call_out(instance);
	free32(instance->td);
	free32(instance->packet);
	free(instance);
}
/**
 * @}
 */
