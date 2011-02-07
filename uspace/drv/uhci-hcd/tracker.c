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

#include <usb/debug.h>

#include "tracker.h"
#include "uhci.h"
#include "utils/malloc32.h"

#define SETUP_PACKET_DATA_SIZE 8
#define DEFAULT_ERROR_COUNT 3
#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a,b) ((a < b) ? a : b)

static int tracker_schedule(tracker_t *instance);

static void tracker_control_read_data(tracker_t *instance);
static void tracker_control_write_data(tracker_t *instance);
static void tracker_control_read_status(tracker_t *instance);
static void tracker_control_write_status(tracker_t *instance);
static void tracker_call_in(tracker_t *instance);
static void tracker_call_out(tracker_t *instance);
static void tracker_call_in_and_dispose(tracker_t *instance);
static void tracker_call_out_and_dispose(tracker_t *instance);


tracker_t * tracker_get(device_t *dev, usb_target_t target,
    usb_transfer_type_t transfer_type, size_t max_packet_size,
    dev_speed_t speed, char *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(func_in == NULL || func_out == NULL);
	assert(func_in != NULL || func_out != NULL);

	tracker_t *instance = malloc(sizeof(tracker_t));
	if (!instance) {
		usb_log_error("Failed to allocate tracker isntance.\n");
		return NULL;
	}

	instance->td = malloc32(sizeof(transfer_descriptor_t));
	if (!instance->td) {
		usb_log_error("Failed to allocate transfer descriptor.\n");
		free(instance);
		return NULL;
	}
	bzero(instance->td, sizeof(transfer_descriptor_t));

	instance->packet = max_packet_size ? malloc32(max_packet_size) : NULL;
	if (max_packet_size && !instance->packet) {
		usb_log_error("Failed to allocate device acessible buffer.\n");
		free32(instance->td);
		free(instance);
		return NULL;
	}
	instance->max_packet_size = max_packet_size;
	instance->packet_size = 0;
	instance->buffer_offset = 0;

	link_initialize(&instance->link);
	instance->target = target;
	instance->transfer_type = transfer_type;

	if (func_out)
		instance->callback_out = func_out;
	if (func_in)
		instance->callback_in = func_in;
	instance->buffer = buffer;
	instance->buffer_size = size;
	instance->dev = dev;
	instance->arg = arg;
	instance->toggle = 0;

	return instance;
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

	tracker_schedule(instance);
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

	tracker_schedule(instance);
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

	//TODO: add toggle here
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_IN,
	    instance->packet);

	tracker_schedule(instance);

	/* set next step */
	if ((instance->buffer_offset + instance->packet_size)
	    >= instance->buffer_size) {
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

	// TODO: add toggle here
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_OUT,
	    instance->packet);

	tracker_schedule(instance);

	/* set next step */
	if ((instance->buffer_offset + instance->packet_size)
	    >= instance->buffer_size) {
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

	tracker_schedule(instance);

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

	tracker_schedule(instance);

	/* set next step, callback and cleanup */
	instance->next_step = tracker_call_out_and_dispose;
}
/*----------------------------------------------------------------------------*/
void tracker_interrupt_in(tracker_t *instance)
{
	assert(instance);

	/* check for errors */
	int err = transfer_descriptor_status(instance->td);
	if (err != EOK) {
		tracker_call_in_and_dispose(instance);
		return;
	}

	if (instance->packet_size) {
		/* we are data in, we want data from our device. if there is data */
		memcpy(instance->buffer + instance->buffer_offset, instance->packet,
				instance->packet_size);
		instance->buffer_offset += instance->packet_size;
	}

	/* prepare next packet, no copy, we are receiving data */
	instance->packet_size =	MIN(instance->max_packet_size,
			instance->buffer_size - instance->buffer_offset);

	//TODO: add toggle here
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_IN,
	    instance->packet);

	tracker_schedule(instance);

	/* set next step */
	if ((instance->buffer_offset + instance->packet_size)
	    >= instance->buffer_size) {
		/* that's all, end coomunication */
		instance->next_step = tracker_call_in_and_dispose;
	}
}
/*----------------------------------------------------------------------------*/
void tracker_interrupt_out(tracker_t *instance)
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

	// TODO: add toggle here
	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_OUT,
	    instance->packet);

	tracker_schedule(instance);

	/* set next step */
	if ((instance->buffer_offset + instance->packet_size)
	    >= instance->buffer_size) {
		/* that's all, end coomunication */
		instance->next_step = tracker_call_out_and_dispose;
	}
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
	instance->buffer_offset += instance->packet_size;
	usb_log_debug("Callback IN: %d, %zu.\n", err, instance->buffer_offset);
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
	usb_log_debug("Callback OUT: %d, %zu.\n", err, instance->buffer_offset);
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
/*----------------------------------------------------------------------------*/
int tracker_schedule(tracker_t *instance)
{
	assert(instance);
	uhci_t *hc = dev_to_uhci(instance->dev);
	assert(hc);
	return uhci_schedule(hc, instance);
}
/*----------------------------------------------------------------------------*/
/* DEPRECATED FUNCTIONS NEEDED BY THE OLD API */
void tracker_control_setup_old(tracker_t *instance)
{
	assert(instance);
	assert(instance->buffer_offset == 0);

	instance->packet_size = SETUP_PACKET_DATA_SIZE;
	memcpy(instance->packet, instance->buffer, SETUP_PACKET_DATA_SIZE);

	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    SETUP_PACKET_DATA_SIZE, false, instance->target, USB_PID_SETUP,
	    instance->packet);

	instance->buffer_offset += SETUP_PACKET_DATA_SIZE;
	instance->next_step = tracker_call_out_and_dispose;

	tracker_schedule(instance);
}

void tracker_control_write_data_old(tracker_t *instance)
{
	assert(instance);
	assert(instance->max_packet_size == instance->buffer_size);

	memcpy(instance->packet, instance->buffer, instance->max_packet_size);
	instance->packet_size = instance->max_packet_size;

	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_OUT,
	    instance->packet);
	instance->next_step = tracker_call_out_and_dispose;

	tracker_schedule(instance);
}

void tracker_control_read_data_old(tracker_t *instance)
{
	assert(instance);
	assert(instance->max_packet_size == instance->buffer_size);

	instance->packet_size = instance->max_packet_size;

	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_IN,
	    instance->packet);

	instance->next_step = tracker_call_in_and_dispose;

	tracker_schedule(instance);
}

void tracker_control_write_status_old(tracker_t *instance)
{
	assert(instance);
	assert(instance->max_packet_size == 0);
	assert(instance->buffer_size == 0);
	assert(instance->packet == NULL);

	instance->packet_size = instance->max_packet_size;
	instance->next_step = tracker_call_in_and_dispose;

	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_IN,
	    instance->packet);

	tracker_schedule(instance);
}

void tracker_control_read_status_old(tracker_t *instance)
{
	assert(instance);
	assert(instance->max_packet_size == 0);
	assert(instance->buffer_size == 0);
	assert(instance->packet == NULL);

	instance->packet_size = instance->max_packet_size;
	instance->next_step = tracker_call_out_and_dispose;

	transfer_descriptor_init(instance->td, DEFAULT_ERROR_COUNT,
	    instance->packet_size, false, instance->target, USB_PID_OUT,
	    instance->packet);

	tracker_schedule(instance);
}
/**
 * @}
 */
