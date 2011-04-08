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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Device keeper structure and functions (implementation).
 */
#include <assert.h>
#include <errno.h>
#include <usb/debug.h>
#include <usb/host/device_keeper.h>

/*----------------------------------------------------------------------------*/
/** Initialize device keeper structure.
 *
 * @param[in] instance Memory place to initialize.
 *
 * Set all values to false/0.
 */
void usb_device_keeper_init(usb_device_keeper_t *instance)
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	fibril_condvar_initialize(&instance->change);
	instance->last_address = 0;
	unsigned i = 0;
	for (; i < USB_ADDRESS_COUNT; ++i) {
		instance->devices[i].occupied = false;
		instance->devices[i].control_used = 0;
		instance->devices[i].handle = 0;
		instance->devices[i].speed = USB_SPEED_MAX;
		list_initialize(&instance->devices[i].endpoints);
	}
	// TODO: is this hack enough?
	// (it is needed to allow smooth registration at default address)
	instance->devices[0].occupied = true;
}
/*----------------------------------------------------------------------------*/
void usb_device_keeper_add_ep(
    usb_device_keeper_t *instance, usb_address_t address, endpoint_t *ep)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	assert(instance->devices[address].occupied);
	list_append(&ep->same_device_eps, &instance->devices[address].endpoints);
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
void usb_device_keeper_del_ep(
    usb_device_keeper_t *instance, usb_address_t address, endpoint_t *ep)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	assert(instance->devices[address].occupied);
	list_remove(&ep->same_device_eps);
	list_initialize(&ep->same_device_eps);
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Attempt to obtain address 0, blocks.
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] speed Speed of the device requesting default address.
 */
void usb_device_keeper_reserve_default_address(
    usb_device_keeper_t *instance, usb_speed_t speed)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	while (instance->devices[USB_ADDRESS_DEFAULT].occupied) {
		fibril_condvar_wait(&instance->change, &instance->guard);
	}
	instance->devices[USB_ADDRESS_DEFAULT].occupied = true;
	instance->devices[USB_ADDRESS_DEFAULT].speed = speed;
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Attempt to obtain address 0, blocks.
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] speed Speed of the device requesting default address.
 */
void usb_device_keeper_release_default_address(usb_device_keeper_t *instance)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	instance->devices[USB_ADDRESS_DEFAULT].occupied = false;
	fibril_mutex_unlock(&instance->guard);
	fibril_condvar_signal(&instance->change);
}
/*----------------------------------------------------------------------------*/
/** Check setup packet data for signs of toggle reset.
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] target Device to receive setup packet.
 * @param[in] data Setup packet data.
 *
 * Really ugly one.
 */
void usb_device_keeper_reset_if_need(
    usb_device_keeper_t *instance, usb_target_t target, const uint8_t *data)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	if (target.endpoint > 15 || target.endpoint < 0
	    || target.address >= USB_ADDRESS_COUNT || target.address < 0
	    || !instance->devices[target.address].occupied) {
		fibril_mutex_unlock(&instance->guard);
		usb_log_error("Invalid data when checking for toggle reset.\n");
		return;
	}

	switch (data[1])
	{
	case 0x01: /*clear feature*/
		/* recipient is endpoint, value is zero (ENDPOINT_STALL) */
		if (((data[0] & 0xf) == 1) && ((data[2] | data[3]) == 0)) {
			link_t *current =
			    instance->devices[target.address].endpoints.next;
			while (current !=
			   &instance->devices[target.address].endpoints)
			{
			/* endpoint number is < 16, thus first byte is enough */
				endpoint_toggle_reset_filtered(
				    current, data[4]);
				current = current->next;
			}
		}
	break;

	case 0x9: /* set configuration */
	case 0x11: /* set interface */
		/* target must be device */
		if ((data[0] & 0xf) == 0) {
			link_t *current =
			    instance->devices[target.address].endpoints.next;
			while (current !=
			   &instance->devices[target.address].endpoints)
			{
				endpoint_toggle_reset(current);
				current = current->next;
			}
		}
	break;
	}
	fibril_mutex_unlock(&instance->guard);
}
/*----------------------------------------------------------------------------*/
/** Get a free USB address
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] speed Speed of the device requiring address.
 * @return Free address, or error code.
 */
usb_address_t device_keeper_get_free_address(
    usb_device_keeper_t *instance, usb_speed_t speed)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);

	usb_address_t new_address = instance->last_address;
	do {
		++new_address;
		if (new_address > USB11_ADDRESS_MAX)
			new_address = 1;
		if (new_address == instance->last_address) {
			fibril_mutex_unlock(&instance->guard);
			return ENOSPC;
		}
	} while (instance->devices[new_address].occupied);

	assert(new_address != USB_ADDRESS_DEFAULT);
	assert(instance->devices[new_address].occupied == false);
	instance->devices[new_address].occupied = true;
	instance->devices[new_address].speed = speed;
	instance->last_address = new_address;
	fibril_mutex_unlock(&instance->guard);
	return new_address;
}
/*----------------------------------------------------------------------------*/
/** Bind USB address to devman handle.
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] address Device address
 * @param[in] handle Devman handle of the device.
 */
void usb_device_keeper_bind(usb_device_keeper_t *instance,
    usb_address_t address, devman_handle_t handle)
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
/** Release used USB address.
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] address Device address
 */
void usb_device_keeper_release(
    usb_device_keeper_t *instance, usb_address_t address)
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
/** Find USB address associated with the device
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] handle Devman handle of the device seeking its address.
 * @return USB Address, or error code.
 */
usb_address_t usb_device_keeper_find(
    usb_device_keeper_t *instance, devman_handle_t handle)
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
/** Get speed associated with the address
 *
 * @param[in] instance Device keeper structure to use.
 * @param[in] address Address of the device.
 * @return USB speed.
 */
usb_speed_t usb_device_keeper_get_speed(
    usb_device_keeper_t *instance, usb_address_t address)
{
	assert(instance);
	assert(address >= 0);
	assert(address <= USB11_ADDRESS_MAX);
	return instance->devices[address].speed;
}
/**
 * @}
 */
