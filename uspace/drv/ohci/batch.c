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

#define DEFAULT_ERROR_COUNT 3
batch_t * batch_get(
    ddf_fun_t *fun,
		usb_target_t target,
    usb_transfer_type_t transfer_type,
		size_t max_packet_size,
    usb_speed_t speed,
		char *buffer,
		size_t buffer_size,
		char *setup_buffer,
		size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
		void *arg,
		device_keeper_t *manager
		)
{
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
	batch_init(instance, target, transfer_type, speed, max_packet_size,
	    buffer, NULL, buffer_size, NULL, setup_size, func_in,
	    func_out, arg, fun, NULL);

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
void batch_dispose(batch_t *instance)
{
	assert(instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_write(batch_t *instance)
{
	assert(instance);
}
/*----------------------------------------------------------------------------*/
void batch_control_read(batch_t *instance)
{
	assert(instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_in(batch_t *instance)
{
	assert(instance);
}
/*----------------------------------------------------------------------------*/
void batch_interrupt_out(batch_t *instance)
{
	assert(instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_in(batch_t *instance)
{
	assert(instance);
}
/*----------------------------------------------------------------------------*/
void batch_bulk_out(batch_t *instance)
{
	assert(instance);
}
/**
 * @}
 */
