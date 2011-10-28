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
 * @brief UHCI host controller driver structure
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <usb/host/endpoint.h>

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
		endpoint_clear_hc_data(instance);
	}
	return instance;
}
/*----------------------------------------------------------------------------*/
void endpoint_destroy(endpoint_t *instance)
{
	assert(instance);
	assert(!instance->active);
	assert(instance->hc_data.data == NULL);
	free(instance);
}
/*----------------------------------------------------------------------------*/
void endpoint_set_hc_data(endpoint_t *instance,
    void *data, int (*toggle_get)(void *), void (*toggle_set)(void *, int))
{
	assert(instance);
	instance->hc_data.data = data;
	instance->hc_data.toggle_get = toggle_get;
	instance->hc_data.toggle_set = toggle_set;
}
/*----------------------------------------------------------------------------*/
void endpoint_clear_hc_data(endpoint_t *instance)
{
	assert(instance);
	instance->hc_data.data = NULL;
	instance->hc_data.toggle_get = NULL;
	instance->hc_data.toggle_set = NULL;
}
/*----------------------------------------------------------------------------*/
void endpoint_use(endpoint_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	while (instance->active)
		fibril_condvar_wait(&instance->avail, &instance->guard);
	instance->active = true;
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
void endpoint_release(endpoint_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	instance->active = false;
	fibril_mutex_unlock(&instance->guard);
	fibril_condvar_signal(&instance->avail);
}
/*----------------------------------------------------------------------------*/
int endpoint_toggle_get(endpoint_t *instance)
{
	assert(instance);
	if (instance->hc_data.toggle_get)
		instance->toggle =
		    instance->hc_data.toggle_get(instance->hc_data.data);
	return (int)instance->toggle;
}
/*----------------------------------------------------------------------------*/
void endpoint_toggle_set(endpoint_t *instance, int toggle)
{
	assert(instance);
	assert(toggle == 0 || toggle == 1);
	if (instance->hc_data.toggle_set)
		instance->hc_data.toggle_set(instance->hc_data.data, toggle);
	instance->toggle = toggle;
}
/**
 * @}
 */
