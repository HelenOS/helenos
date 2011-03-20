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
/** @addtogroup drvusbohcihc
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

static void batch_call_in(batch_t *instance);
static void batch_call_out(batch_t *instance);
static void batch_call_in_and_dispose(batch_t *instance);
static void batch_call_out_and_dispose(batch_t *instance);

/** Allocate memory and initialize internal data structure.
 *
 * @param[in] fun DDF function to pass to callback.
 * @param[in] target Device and endpoint target of the transaction.
 * @param[in] transfer_type Interrupt, Control or Bulk.
 * @param[in] max_packet_size maximum allowed size of data packets.
 * @param[in] speed Speed of the transaction.
 * @param[in] buffer Data source/destination.
 * @param[in] size Size of the buffer.
 * @param[in] setup_buffer Setup data source (if not NULL)
 * @param[in] setup_size Size of setup_buffer (should be always 8)
 * @param[in] func_in function to call on inbound transaction completion
 * @param[in] func_out function to call on outbound transaction completion
 * @param[in] arg additional parameter to func_in or func_out
 * @param[in] manager Pointer to toggle management structure.
 * @return Valid pointer if all substructures were successfully created,
 * NULL otherwise.
 *
 * Determines the number of needed packets (TDs). Prepares a transport buffer
 * (that is accessible by the hardware). Initializes parameters needed for the
 * transaction and callback.
 */
batch_t * batch_get(ddf_fun_t *fun, usb_target_t target,
    usb_transfer_type_t transfer_type, size_t max_packet_size,
    usb_speed_t speed, char *buffer, size_t size,
    char* setup_buffer, size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg
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

	if (size > 0) {
		/* TODO: use device accessible malloc here */
		instance->transport_buffer = malloc(size);
		CHECK_NULL_DISPOSE_RETURN(instance->transport_buffer,
		    "Failed to allocate device accessible buffer.\n");
	}

	if (setup_size > 0) {
		/* TODO: use device accessible malloc here */
		instance->setup_buffer = malloc(setup_size);
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
	instance->callback_out = func_out;
	instance->callback_in = func_in;

	usb_log_debug("Batch(%p) %d:%d memory structures ready.\n",
	    instance, target.address, target.endpoint);
	return instance;
}
/*----------------------------------------------------------------------------*/
/** Mark batch as finished and continue with next step.
 *
 * @param[in] instance Batch structure to use.
 *
 */
void batch_finish(batch_t *instance, int error)
{
	assert(instance);
	instance->error = error;
	instance->next_step(instance);
}
/*----------------------------------------------------------------------------*/
/** Check batch TDs for activity.
 *
 * @param[in] instance Batch structure to use.
 * @return False, if there is an active TD, true otherwise.
 *
 * Walk all TDs. Stop with false if there is an active one (it is to be
 * processed). Stop with true if an error is found. Return true if the last TS
 * is reached.
 */
bool batch_is_complete(batch_t *instance)
{
	assert(instance);
	/* TODO: implement */
	return true;
}
/*----------------------------------------------------------------------------*/
/** Prepares control write transaction.
 *
 * @param[in] instance Batch structure to use.
 *
 * Uses genercir control function with pids OUT and IN.
 */
void batch_control_write(batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer,
	    instance->buffer_size);
	instance->next_step = batch_call_out_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) CONTROL WRITE initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepares control read transaction.
 *
 * @param[in] instance Batch structure to use.
 *
 * Uses generic control with pids IN and OUT.
 */
void batch_control_read(batch_t *instance)
{
	assert(instance);
	instance->next_step = batch_call_in_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) CONTROL READ initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare interrupt in transaction.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transaction with PID_IN.
 */
void batch_interrupt_in(batch_t *instance)
{
	assert(instance);
	/* TODO: implement */
	instance->next_step = batch_call_in_and_dispose;
	usb_log_debug("Batch(%p) INTERRUPT IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare interrupt out transaction.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transaction with PID_OUT.
 */
void batch_interrupt_out(batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer,
	    instance->buffer_size);
	instance->next_step = batch_call_out_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) INTERRUPT OUT initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare bulk in transaction.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transaction with PID_IN.
 */
void batch_bulk_in(batch_t *instance)
{
	assert(instance);
	instance->next_step = batch_call_in_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) BULK IN initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare bulk out transaction.
 *
 * @param[in] instance Batch structure to use.
 *
 * Data transaction with PID_OUT.
 */
void batch_bulk_out(batch_t *instance)
{
	assert(instance);
	/* We are data out, we are supposed to provide data */
	memcpy(instance->transport_buffer, instance->buffer,
	    instance->buffer_size);
	instance->next_step = batch_call_out_and_dispose;
	/* TODO: implement */
	usb_log_debug("Batch(%p) BULK OUT initialized.\n", instance);
}
/*----------------------------------------------------------------------------*/
/** Prepare data, get error status and call callback in.
 *
 * @param[in] instance Batch structure to use.
 * Copies data from transport buffer, and calls callback with appropriate
 * parameters.
 */
void batch_call_in(batch_t *instance)
{
	assert(instance);
	assert(instance->callback_in);

	/* We are data in, we need data */
	memcpy(instance->buffer, instance->transport_buffer,
	    instance->buffer_size);

	int err = instance->error;
	usb_log_debug("Batch(%p) callback IN(type:%d): %s(%d), %zu.\n",
	    instance, instance->transfer_type, str_error(err), err,
	    instance->transfered_size);

	instance->callback_in(
	    instance->fun, err, instance->transfered_size, instance->arg);
}
/*----------------------------------------------------------------------------*/
/** Get error status and call callback out.
 *
 * @param[in] instance Batch structure to use.
 */
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
/** Helper function calls callback and correctly disposes of batch structure.
 *
 * @param[in] instance Batch structure to use.
 */
void batch_call_in_and_dispose(batch_t *instance)
{
	assert(instance);
	batch_call_in(instance);
	batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
/** Helper function calls callback and correctly disposes of batch structure.
 *
 * @param[in] instance Batch structure to use.
 */
void batch_call_out_and_dispose(batch_t *instance)
{
	assert(instance);
	batch_call_out(instance);
	batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
/** Correctly dispose all used data structures.
 *
 * @param[in] instance Batch structure to use.
 */
void batch_dispose(batch_t *instance)
{
	assert(instance);
	usb_log_debug("Batch(%p) disposing.\n", instance);
	if (instance->setup_buffer)
		free(instance->setup_buffer);
	if (instance->transport_buffer)
		free(instance->transport_buffer);
	free(instance);
}
/**
 * @}
 */
