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

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <assert.h>
#include <errno.h>

#include "device_keeper.h"

/*----------------------------------------------------------------------------*/
void device_keeper_init(device_keeper_t *instance)
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	fibril_condvar_initialize(&instance->default_address_occupied);
	instance->last_address = 0;
	unsigned i = 0;
	for (; i < USB_ADDRESS_COUNT; ++i) {
		instance->devices[i].occupied = false;
		instance->devices[i].handle = 0;
	}
}
/*----------------------------------------------------------------------------*/
void device_keeper_reserve_default(
    device_keeper_t *instance, usb_speed_t speed)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	while (instance->devices[USB_ADDRESS_DEFAULT].occupied) {
		fibril_condvar_wait(&instance->default_address_occupied,
		    &instance->guard);
	}
	instance->devices[USB_ADDRESS_DEFAULT].occupied = true;
	instance->devices[USB_ADDRESS_DEFAULT].speed = speed;
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
void device_keeper_release_default(device_keeper_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	instance->devices[USB_ADDRESS_DEFAULT].occupied = false;
	fibril_mutex_unlock(&instance->guard);
	fibril_condvar_signal(&instance->default_address_occupied);
}
/*----------------------------------------------------------------------------*/
usb_address_t device_keeper_request(
    device_keeper_t *instance, usb_speed_t speed)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);

	usb_address_t new_address = instance->last_address + 1;
	while (instance->devices[new_address].occupied) {
		if (new_address == instance->last_address) {
			fibril_mutex_unlock(&instance->guard);
			return ENOSPC;
		}
		if (new_address == USB11_ADDRESS_MAX)
			new_address = 1;
		++new_address;
	}

	assert(new_address != USB_ADDRESS_DEFAULT);
	assert(instance->devices[new_address].occupied == false);
	instance->devices[new_address].occupied = true;
	instance->devices[new_address].speed = speed;
	instance->last_address = new_address;
	fibril_mutex_unlock(&instance->guard);
	return new_address;
}
/*----------------------------------------------------------------------------*/
void device_keeper_bind(
    device_keeper_t *instance, usb_address_t address, devman_handle_t handle)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	assert(address > 0);
	assert(address <= USB11_ADDRESS_MAX);
	assert(instance->devices[address].occupied);
	instance->devices[address].handle = handle;
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
void device_keeper_release(device_keeper_t *instance, usb_address_t address)
{
	assert(instance);
	assert(address > 0);
	assert(address <= USB11_ADDRESS_MAX);

	fibril_mutex_lock(&instance->guard);
	assert(instance->devices[address].occupied);
	instance->devices[address].occupied = false;
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
usb_address_t device_keeper_find(
    device_keeper_t *instance, devman_handle_t handle)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	usb_address_t address = 1;
	while (address <= USB11_ADDRESS_MAX) {
		if (instance->devices[address].handle == handle) {
			fibril_mutex_unlock(&instance->guard);
			return address;
		}
		++address;
	}
	fibril_mutex_unlock(&instance->guard);
	return ENOENT;
}
/*----------------------------------------------------------------------------*/
usb_speed_t device_keeper_speed(
    device_keeper_t *instance, usb_address_t address)
{
	assert(instance);
	assert(address >= 0);
	assert(address <= USB11_ADDRESS_MAX);
	return instance->devices[address].speed;
}
/**
 * @}
 */
