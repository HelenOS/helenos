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
 * Device manager structure and functions (implementation).
 */
#include <assert.h>
#include <errno.h>
#include <usb/debug.h>
#include <usb/host/usb_device_manager.h>

/** Get a free USB address
 *
 * @param[in] instance Device manager structure to use.
 * @return Free address, or error code.
 */
static usb_address_t usb_device_manager_get_free_address(
    usb_device_manager_t *instance)
{

	usb_address_t new_address = instance->last_address;
	do {
		new_address = (new_address + 1) % USB_ADDRESS_COUNT;
		if (new_address == USB_ADDRESS_DEFAULT)
			new_address = 1;
		if (new_address == instance->last_address) {
			return ENOSPC;
		}
	} while (instance->devices[new_address].occupied);

	assert(new_address != USB_ADDRESS_DEFAULT);
	instance->last_address = new_address;

	return new_address;
}

/** Initialize device manager structure.
 *
 * @param[in] instance Memory place to initialize.
 * @param[in] max_speed Maximum allowed USB speed of devices (inclusive).
 *
 * Set all values to false/0.
 */
void usb_device_manager_init(
    usb_device_manager_t *instance, usb_speed_t max_speed)
{
	assert(instance);
	for (unsigned i = 0; i < USB_ADDRESS_COUNT; ++i) {
		instance->devices[i].occupied = false;
		instance->devices[i].handle = 0;
		instance->devices[i].speed = USB_SPEED_MAX;
	}
	instance->last_address = 1;
	instance->max_speed = max_speed;
	fibril_mutex_initialize(&instance->guard);
}

/** Request USB address.
 * @param instance usb_device_manager
 * @param address Pointer to requested address value, place to store new address
 * @parma strict Fail if the requested address is not available.
 * @return Error code.
 * @note Default address is only available in strict mode.
 */
int usb_device_manager_request_address(usb_device_manager_t *instance,
    usb_address_t *address, bool strict, usb_speed_t speed)
{
	assert(instance);
	assert(address);
	if (speed > instance->max_speed)
		return ENOTSUP;

	if ((*address) < 0 || (*address) >= USB_ADDRESS_COUNT)
		return EINVAL;

	fibril_mutex_lock(&instance->guard);
	/* Only grant default address to strict requests */
	if (( (*address) == USB_ADDRESS_DEFAULT) && !strict) {
		*address = instance->last_address;
	}

	if (instance->devices[*address].occupied) {
		if (strict) {
			fibril_mutex_unlock(&instance->guard);
			return ENOENT;
		}
		*address = usb_device_manager_get_free_address(instance);
	}
	assert(instance->devices[*address].occupied == false);
	assert(instance->devices[*address].handle == 0);
	assert(*address != USB_ADDRESS_DEFAULT || strict);

	instance->devices[*address].occupied = true;
	instance->devices[*address].speed = speed;

	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Bind USB address to devman handle.
 *
 * @param[in] instance Device manager structure to use.
 * @param[in] address Device address
 * @param[in] handle Devman handle of the device.
 * @return Error code.
 * @note Won't accept binding for default address.
 */
int usb_device_manager_bind_address(usb_device_manager_t *instance,
    usb_address_t address, devman_handle_t handle)
{
	if ((address <= 0) || (address >= USB_ADDRESS_COUNT)) {
		return EINVAL;
	}
	assert(instance);

	fibril_mutex_lock(&instance->guard);
	/* Not reserved */
	if (!instance->devices[address].occupied) {
		fibril_mutex_unlock(&instance->guard);
		return ENOENT;
	}
	/* Already bound */
	if (instance->devices[address].handle != 0) {
		fibril_mutex_unlock(&instance->guard);
		return EEXISTS;
	}
	instance->devices[address].handle = handle;
	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Release used USB address.
 *
 * @param[in] instance Device manager structure to use.
 * @param[in] address Device address
 * @return Error code.
 */
int usb_device_manager_release_address(
    usb_device_manager_t *instance, usb_address_t address)
{
	if ((address < 0) || (address >= USB_ADDRESS_COUNT)) {
		return EINVAL;
	}
	assert(instance);

	fibril_mutex_lock(&instance->guard);
	if (!instance->devices[address].occupied) {
		fibril_mutex_unlock(&instance->guard);
		return ENOENT;
	}

	instance->devices[address].occupied = false;
	instance->devices[address].handle = 0;
	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Find USB address associated with the device.
 *
 * @param[in] instance Device manager structure to use.
 * @param[in] handle Devman handle of the device seeking its address.
 * @return USB Address, or error code.
 */
usb_address_t usb_device_manager_find_address(
    usb_device_manager_t *instance, devman_handle_t handle)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	for (usb_address_t address = 1; address <= USB11_ADDRESS_MAX; ++address)
	{
		if (instance->devices[address].handle == handle) {
			assert(instance->devices[address].occupied);
			fibril_mutex_unlock(&instance->guard);
			return address;
		}
	}
	fibril_mutex_unlock(&instance->guard);
	return ENOENT;
}

/** Find devman handle and speed assigned to USB address.
 *
 * @param[in] instance Device manager structure to use.
 * @param[in] address Address the caller wants to find.
 * @param[out] handle Where to store found handle.
 * @param[out] speed Assigned speed.
 * @return Error code.
 */
int usb_device_manager_get_info_by_address(usb_device_manager_t *instance,
    usb_address_t address, devman_handle_t *handle, usb_speed_t *speed)
{
	assert(instance);
	if ((address < 0) || (address >= USB_ADDRESS_COUNT)) {
		return EINVAL;
	}

	fibril_mutex_lock(&instance->guard);
	if (!instance->devices[address].occupied) {
		fibril_mutex_unlock(&instance->guard);
		return ENOENT;
	}

	if (handle != NULL) {
		*handle = instance->devices[address].handle;
	}
	if (speed != NULL) {
		*speed = instance->devices[address].speed;
	}

	fibril_mutex_unlock(&instance->guard);
	return EOK;
}
/**
 * @}
 */
