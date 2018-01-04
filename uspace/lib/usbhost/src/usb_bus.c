/*
 * Copyright (c) 2011 Jan Vesely
 * All rights eps.
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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 * HC Endpoint management.
 */

#include <usb/host/usb_bus.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <stdbool.h>


/** Endpoint compare helper function.
 *
 * USB_DIRECTION_BOTH matches both IN and OUT.
 * @param ep Endpoint to compare, non-null.
 * @param address Tested address.
 * @param endpoint Tested endpoint number.
 * @param direction Tested direction.
 * @return True if ep can be used to communicate with given device,
 * false otherwise.
 */
static inline bool ep_match(const endpoint_t *ep,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(ep);
	return
	    ((direction == ep->direction)
	        || (ep->direction == USB_DIRECTION_BOTH)
	        || (direction == USB_DIRECTION_BOTH))
	    && (endpoint == ep->endpoint)
	    && (address == ep->address);
}

/** Get list that holds endpoints for given address.
 * @param instance usb_bus structure, non-null.
 * @param addr USB address, must be >= 0.
 * @return Pointer to the appropriate list.
 */
static list_t * get_list(usb_bus_t *instance, usb_address_t addr)
{
	assert(instance);
	assert(addr >= 0);
	return &instance->devices[addr % ARRAY_SIZE(instance->devices)].endpoint_list;
}

/** Internal search function, works on locked structure.
 * @param instance usb_bus structure, non-null.
 * @param address USB address, must be valid.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @return Pointer to endpoint_t structure representing given communication
 * target, NULL if there is no such endpoint registered.
 * @note Assumes that the internal mutex is locked.
 */
static endpoint_t * find_locked(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);
	assert(fibril_mutex_is_locked(&instance->guard));
	if (address < 0)
		return NULL;
	list_foreach(*get_list(instance, address), link, endpoint_t, ep) {
		if (ep_match(ep, address, endpoint, direction))
			return ep;
	}
	return NULL;
}

/** Get a free USB address
 *
 * @param[in] instance Device manager structure to use.
 * @param[out] address Free address.
 * @return Error code.
 */
static errno_t usb_bus_get_free_address(usb_bus_t *instance, usb_address_t *address)
{

	usb_address_t new_address = instance->last_address;
	do {
		new_address = (new_address + 1) % USB_ADDRESS_COUNT;
		if (new_address == USB_ADDRESS_DEFAULT)
			new_address = 1;
		if (new_address == instance->last_address)
			return ENOSPC;
	} while (instance->devices[new_address].occupied);

	assert(new_address != USB_ADDRESS_DEFAULT);
	instance->last_address = new_address;

	*address = new_address;
	return EOK;
}

/** Calculate bandwidth that needs to be reserved for communication with EP.
 * Calculation follows USB 1.1 specification.
 * @param speed Device's speed.
 * @param type Type of the transfer.
 * @param size Number of byte to transfer.
 * @param max_packet_size Maximum bytes in one packet.
 */
size_t bandwidth_count_usb11(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size)
{
	/* We care about bandwidth only for interrupt and isochronous. */
	if ((type != USB_TRANSFER_INTERRUPT)
	    && (type != USB_TRANSFER_ISOCHRONOUS)) {
		return 0;
	}

	const unsigned packet_count =
	    (size + max_packet_size - 1) / max_packet_size;
	/* TODO: It may be that ISO and INT transfers use only one packet per
	 * transaction, but I did not find text in USB spec to confirm this */
	/* NOTE: All data packets will be considered to be max_packet_size */
	switch (speed)
	{
	case USB_SPEED_LOW:
		assert(type == USB_TRANSFER_INTERRUPT);
		/* Protocol overhead 13B
		 * (3 SYNC bytes, 3 PID bytes, 2 Endpoint + CRC bytes, 2
		 * CRC bytes, and a 3-byte interpacket delay)
		 * see USB spec page 45-46. */
		/* Speed penalty 8: low speed is 8-times slower*/
		return packet_count * (13 + max_packet_size) * 8;
	case USB_SPEED_FULL:
		/* Interrupt transfer overhead see above
		 * or page 45 of USB spec */
		if (type == USB_TRANSFER_INTERRUPT)
			return packet_count * (13 + max_packet_size);

		assert(type == USB_TRANSFER_ISOCHRONOUS);
		/* Protocol overhead 9B
		 * (2 SYNC bytes, 2 PID bytes, 2 Endpoint + CRC bytes, 2 CRC
		 * bytes, and a 1-byte interpacket delay)
		 * see USB spec page 42 */
		return packet_count * (9 + max_packet_size);
	default:
		return 0;
	}
}

/** Calculate bandwidth that needs to be reserved for communication with EP.
 * Calculation follows USB 2.0 specification.
 * @param speed Device's speed.
 * @param type Type of the transfer.
 * @param size Number of byte to transfer.
 * @param max_packet_size Maximum bytes in one packet.
 */
size_t bandwidth_count_usb20(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size)
{
	/* We care about bandwidth only for interrupt and isochronous. */
	if ((type != USB_TRANSFER_INTERRUPT)
	    && (type != USB_TRANSFER_ISOCHRONOUS)) {
		return 0;
	}
	//TODO Implement
	return 0;
}

/** Initialize to default state.
 * You need to provide valid bw_count function if you plan to use
 * add_endpoint/remove_endpoint pair.
 *
 * @param instance usb_bus structure, non-null.
 * @param available_bandwidth Size of the bandwidth pool.
 * @param bw_count function to use to calculate endpoint bw requirements.
 * @return Error code.
 */
errno_t usb_bus_init(usb_bus_t *instance,
    size_t available_bandwidth, bw_count_func_t bw_count, usb_speed_t max_speed)
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	instance->free_bw = available_bandwidth;
	instance->bw_count = bw_count;
	instance->last_address = 0;
	instance->max_speed = max_speed;
	for (unsigned i = 0; i < ARRAY_SIZE(instance->devices); ++i) {
		list_initialize(&instance->devices[i].endpoint_list);
		instance->devices[i].speed = USB_SPEED_MAX;
		instance->devices[i].occupied = false;
	}
	return EOK;
}

/** Register endpoint structure.
 * Checks for duplicates.
 * @param instance usb_bus, non-null.
 * @param ep endpoint_t to register.
 * @param data_size Size of data to transfer.
 * @return Error code.
 */
errno_t usb_bus_register_ep(usb_bus_t *instance, endpoint_t *ep, size_t data_size)
{
	assert(instance);
	if (ep == NULL || ep->address < 0)
		return EINVAL;

	fibril_mutex_lock(&instance->guard);
	/* Check for available bandwidth */
	if (ep->bandwidth > instance->free_bw) {
		fibril_mutex_unlock(&instance->guard);
		return ENOSPC;
	}

	/* Check for existence */
	const endpoint_t *endpoint =
	    find_locked(instance, ep->address, ep->endpoint, ep->direction);
	if (endpoint != NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EEXIST;
	}
	/* Add endpoint list's reference to ep. */
	endpoint_add_ref(ep);
	list_append(&ep->link, get_list(instance, ep->address));

	instance->free_bw -= ep->bandwidth;
	usb_log_debug("Registered EP(%d:%d:%s:%s)\n", ep->address, ep->endpoint,
	    usb_str_transfer_type_short(ep->transfer_type),
	    usb_str_direction(ep->direction));
	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Unregister endpoint structure.
 * Checks for duplicates.
 * @param instance usb_bus, non-null.
 * @param ep endpoint_t to unregister.
 * @return Error code.
 */
errno_t usb_bus_unregister_ep(usb_bus_t *instance, endpoint_t *ep)
{
	assert(instance);
	if (ep == NULL || ep->address < 0)
		return EINVAL;

	fibril_mutex_lock(&instance->guard);
	if (!list_member(&ep->link, get_list(instance, ep->address))) {
		fibril_mutex_unlock(&instance->guard);
		return ENOENT;
	}
	list_remove(&ep->link);
	instance->free_bw += ep->bandwidth;
	usb_log_debug("Unregistered EP(%d:%d:%s:%s)\n", ep->address,
	    ep->endpoint, usb_str_transfer_type_short(ep->transfer_type),
	    usb_str_direction(ep->direction));
	/* Drop endpoint list's reference to ep. */
	endpoint_del_ref(ep);
	fibril_mutex_unlock(&instance->guard);
	return EOK;
}

/** Find endpoint_t representing the given communication route.
 * @param instance usb_bus, non-null.
 * @param address
 */
endpoint_t * usb_bus_find_ep(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);

	fibril_mutex_lock(&instance->guard);
	endpoint_t *ep = find_locked(instance, address, endpoint, direction);
	if (ep) {
		/* We are exporting ep to the outside world, add reference. */
		endpoint_add_ref(ep);
	}
	fibril_mutex_unlock(&instance->guard);
	return ep;
}

/** Create and register new endpoint_t structure.
 * @param instance usb_bus structure, non-null.
 * @param address USB address.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @param type USB transfer type.
 * @param speed USB Communication speed.
 * @param max_packet_size Maximum size of data packets.
 * @param data_size Expected communication size.
 * @param callback function to call just after registering.
 * @param arg Argument to pass to the callback function.
 * @return Error code.
 */
errno_t usb_bus_add_ep(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    usb_transfer_type_t type, size_t max_packet_size, unsigned packets,
    size_t data_size, ep_add_callback_t callback, void *arg,
    usb_address_t tt_address, unsigned tt_port)
{
	assert(instance);
	if (instance->bw_count == NULL)
		return ENOTSUP;
	if (!usb_address_is_valid(address))
		return EINVAL;


	fibril_mutex_lock(&instance->guard);
	/* Check for speed and address */
	if (!instance->devices[address].occupied) {
		fibril_mutex_unlock(&instance->guard);
		return ENOENT;
	}

	/* Check for existence */
	endpoint_t *ep = find_locked(instance, address, endpoint, direction);
	if (ep != NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EEXIST;
	}

	const usb_speed_t speed = instance->devices[address].speed;
	const size_t bw =
	    instance->bw_count(speed, type, data_size, max_packet_size);

	/* Check for available bandwidth */
	if (bw > instance->free_bw) {
		fibril_mutex_unlock(&instance->guard);
		return ENOSPC;
	}

	ep = endpoint_create(address, endpoint, direction, type, speed,
	    max_packet_size, packets, bw, tt_address, tt_port);
	if (!ep) {
		fibril_mutex_unlock(&instance->guard);
		return ENOMEM;
	}

	/* Add our reference to ep. */
	endpoint_add_ref(ep);

	if (callback) {
		const errno_t ret = callback(ep, arg);
		if (ret != EOK) {
			fibril_mutex_unlock(&instance->guard);
			endpoint_del_ref(ep);
			return ret;
		}
	}
	
	/* Add endpoint list's reference to ep. */
	endpoint_add_ref(ep);
	list_append(&ep->link, get_list(instance, ep->address));

	instance->free_bw -= ep->bandwidth;
	fibril_mutex_unlock(&instance->guard);

	/* Drop our reference to ep. */
	endpoint_del_ref(ep);

	return EOK;
}

/** Unregister and destroy endpoint_t structure representing given route.
 * @param instance usb_bus structure, non-null.
 * @param address USB address.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @param callback Function to call after unregister, before destruction.
 * @arg Argument to pass to the callback function.
 * @return Error code.
 */
errno_t usb_bus_remove_ep(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    ep_remove_callback_t callback, void *arg)
{
	assert(instance);
	fibril_mutex_lock(&instance->guard);
	endpoint_t *ep = find_locked(instance, address, endpoint, direction);
	if (ep != NULL) {
		list_remove(&ep->link);
		instance->free_bw += ep->bandwidth;
	}
	fibril_mutex_unlock(&instance->guard);
	if (ep == NULL)
		return ENOENT;

	if (callback) {
		callback(ep, arg);
	}
	/* Drop endpoint list's reference to ep. */
	endpoint_del_ref(ep);
	return EOK;
}

errno_t usb_bus_reset_toggle(usb_bus_t *instance, usb_target_t target, bool all)
{
	assert(instance);
	if (!usb_target_is_valid(target))
		return EINVAL;

	errno_t ret = ENOENT;

	fibril_mutex_lock(&instance->guard);
	list_foreach(*get_list(instance, target.address), link, endpoint_t, ep) {
		if ((ep->address == target.address)
		    && (all || ep->endpoint == target.endpoint)) {
			endpoint_toggle_set(ep, 0);
			ret = EOK;
		}
	}
	fibril_mutex_unlock(&instance->guard);
	return ret;
}

/** Unregister and destroy all endpoints using given address.
 * @param instance usb_bus structure, non-null.
 * @param address USB address.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction.
 * @param callback Function to call after unregister, before destruction.
 * @arg Argument to pass to the callback function.
 * @return Error code.
 */
errno_t usb_bus_remove_address(usb_bus_t *instance,
    usb_address_t address, ep_remove_callback_t callback, void *arg)
{
	assert(instance);
	if (!usb_address_is_valid(address))
		return EINVAL;

	fibril_mutex_lock(&instance->guard);

	const errno_t ret = instance->devices[address].occupied ? EOK : ENOENT;
	instance->devices[address].occupied = false;

	list_t *list = get_list(instance, address);
	for (link_t *link = list_first(list); link != NULL; ) {
		endpoint_t *ep = list_get_instance(link, endpoint_t, link);
		link = list_next(link, list);
		if (ep->address == address) {
			list_remove(&ep->link);
			if (callback)
				callback(ep, arg);
			/* Drop endpoint list's reference to ep. */
			endpoint_del_ref(ep);
		}
	}
	fibril_mutex_unlock(&instance->guard);
	return ret;
}

/** Request USB address.
 * @param instance usb_device_manager
 * @param[inout] address Pointer to requested address value, place to store new address
 * @parma strict Fail if the requested address is not available.
 * @return Error code.
 * @note Default address is only available in strict mode.
 */
errno_t usb_bus_request_address(usb_bus_t *instance,
    usb_address_t *address, bool strict, usb_speed_t speed)
{
	assert(instance);
	assert(address);
	if (speed > instance->max_speed)
		return ENOTSUP;

	if (!usb_address_is_valid(*address))
		return EINVAL;

	usb_address_t addr = *address;
	errno_t rc;

	fibril_mutex_lock(&instance->guard);
	/* Only grant default address to strict requests */
	if ((addr == USB_ADDRESS_DEFAULT) && !strict) {
		rc = usb_bus_get_free_address(instance, &addr);
		if (rc != EOK) {
			fibril_mutex_unlock(&instance->guard);
			return rc;
		}
	}

	if (instance->devices[addr].occupied) {
		if (strict) {
			fibril_mutex_unlock(&instance->guard);
			return ENOENT;
		}
		rc = usb_bus_get_free_address(instance, &addr);
		if (rc != EOK) {
			fibril_mutex_unlock(&instance->guard);
			return rc;
		}
	}
	if (usb_address_is_valid(addr)) {
		assert(instance->devices[addr].occupied == false);
		assert(addr != USB_ADDRESS_DEFAULT || strict);

		instance->devices[addr].occupied = true;
		instance->devices[addr].speed = speed;
		*address = addr;
		addr = 0;
	}

	fibril_mutex_unlock(&instance->guard);

	*address = addr;
	return EOK;
}

/** Get speed assigned to USB address.
 *
 * @param[in] instance Device manager structure to use.
 * @param[in] address Address the caller wants to find.
 * @param[out] speed Assigned speed.
 * @return Error code.
 */
errno_t usb_bus_get_speed(usb_bus_t *instance, usb_address_t address,
    usb_speed_t *speed)
{
	assert(instance);
	if (!usb_address_is_valid(address)) {
		return EINVAL;
	}

	fibril_mutex_lock(&instance->guard);

	const errno_t ret = instance->devices[address].occupied ? EOK : ENOENT;
	if (speed && instance->devices[address].occupied) {
		*speed = instance->devices[address].speed;
	}

	fibril_mutex_unlock(&instance->guard);
	return ret;
}
/**
 * @}
 */
