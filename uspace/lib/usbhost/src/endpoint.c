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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 * @brief UHCI host controller driver structure
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <usb/host/endpoint.h>

/** Allocate ad initialize endpoint_t structure.
 * @param address USB address.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @param type USB transfer type.
 * @param speed Communication speed.
 * @param max_packet_size Maximum size of data packets.
 * @param bw Required bandwidth.
 * @return Pointer to initialized endpoint_t structure, NULL on failure.
 */
endpoint_t * endpoint_create(usb_address_t address, usb_endpoint_t endpoint,
    usb_direction_t direction, usb_transfer_type_t type, usb_speed_t speed,
    size_t max_packet_size, size_t bw)
{
	endpoint_t *instance = malloc(sizeof(endpoint_t));
	if (instance) {
		instance->address = address;
		instance->endpoint = endpoint;
		instance->direction = direction;
		instance->transfer_type = type;
		instance->speed = speed;
		instance->max_packet_size = max_packet_size;
		instance->bandwidth = bw;
		instance->toggle = 0;
		instance->active = false;
		instance->hc_data.data = NULL;
		instance->hc_data.toggle_get = NULL;
		instance->hc_data.toggle_set = NULL;
		link_initialize(&instance->link);
		fibril_mutex_initialize(&instance->guard);
		fibril_condvar_initialize(&instance->avail);
	}
	return instance;
}

/** Properly dispose of endpoint_t structure.
 * @param instance endpoint_t structure.
 */
void endpoint_destroy(endpoint_t *instance)
{
	assert(instance);
	//TODO: Do something about waiting fibrils.
	assert(!instance->active);
	assert(instance->hc_data.data == NULL);
	free(instance);
}

/** Set device specific data and hooks.
 * @param instance endpoint_t structure.
 * @param data device specific data.
 * @param toggle_get Hook to call when retrieving value of toggle bit.
 * @param toggle_set Hook to call when setting the value of toggle bit.
 */
void endpoint_set_hc_data(endpoint_t *instance,
    void *data, int (*toggle_get)(void *), void (*toggle_set)(void *, int))
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	instance->hc_data.data = data;
	instance->hc_data.toggle_get = toggle_get;
	instance->hc_data.toggle_set = toggle_set;
	fibril_mutex_unlock(&instance->guard);
}

/** Clear device specific data and hooks.
 * @param instance endpoint_t structure.
 * @note This function does not free memory pointed to by data pointer.
 */
void endpoint_clear_hc_data(endpoint_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	instance->hc_data.data = NULL;
	instance->hc_data.toggle_get = NULL;
	instance->hc_data.toggle_set = NULL;
	fibril_mutex_unlock(&instance->guard);
}

/** Mark the endpoint as active and block access for further fibrils.
 * @param instance endpoint_t structure.
 */
void endpoint_use(endpoint_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	while (instance->active)
		fibril_condvar_wait(&instance->avail, &instance->guard);
	instance->active = true;
	fibril_mutex_unlock(&instance->guard);
}

/** Mark the endpoint as inactive and allow access for further fibrils.
 * @param instance endpoint_t structure.
 */
void endpoint_release(endpoint_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	instance->active = false;
	fibril_mutex_unlock(&instance->guard);
	fibril_condvar_signal(&instance->avail);
}

/** Get the value of toggle bit.
 * @param instance endpoint_t structure.
 * @note Will use provided hook.
 */
int endpoint_toggle_get(endpoint_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	if (instance->hc_data.toggle_get)
		instance->toggle =
		    instance->hc_data.toggle_get(instance->hc_data.data);
	const int ret = instance->toggle;
	fibril_mutex_unlock(&instance->guard);
	return ret;
}

/** Set the value of toggle bit.
 * @param instance endpoint_t structure.
 * @note Will use provided hook.
 */
void endpoint_toggle_set(endpoint_t *instance, int toggle)
{
	assert(instance);
	assert(toggle == 0 || toggle == 1);
	fibril_mutex_lock(&instance->guard);
	instance->toggle = toggle;
	if (instance->hc_data.toggle_set)
		instance->hc_data.toggle_set(instance->hc_data.data, toggle);
	fibril_mutex_unlock(&instance->guard);
}
/**
 * @}
 */
