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

#include <bool.h>
#include <assert.h>
#include <errno.h>

#include <usb/debug.h>
#include <usb/host/usb_endpoint_manager.h>

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
/*----------------------------------------------------------------------------*/
static list_t * get_list(usb_endpoint_manager_t *instance, usb_address_t addr)
{
	assert(instance);
	return &instance->endpoint_lists[addr % ENDPOINT_LIST_COUNT];
}
/*----------------------------------------------------------------------------*/
static endpoint_t * find_locked(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);
	assert(fibril_mutex_is_locked(&instance->guard));
	list_foreach(*get_list(instance, address), iterator) {
		endpoint_t *ep = endpoint_get_instance(iterator);
		if (ep_match(ep, address, endpoint, direction))
			return ep;
	}
	return NULL;
}
/*----------------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------------*/
int usb_endpoint_manager_init(usb_endpoint_manager_t *instance,
    size_t available_bandwidth,
    size_t (*bw_count)(usb_speed_t, usb_transfer_type_t, size_t, size_t))
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	instance->free_bw = available_bandwidth;
	instance->bw_count = bw_count;
	for (unsigned i = 0; i < ENDPOINT_LIST_COUNT; ++i) {
		list_initialize(&instance->endpoint_lists[i]);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
int usb_endpoint_manager_register_ep(usb_endpoint_manager_t *instance,
    endpoint_t *ep, size_t data_size)
{
	assert(instance);
	assert(instance->bw_count);
	assert(ep);
	ep->bandwidth = instance->bw_count(ep->speed, ep->transfer_type,
	    data_size, ep->max_packet_size);

	fibril_mutex_lock(&instance->guard);

	if (ep->bandwidth > instance->free_bw) {
		fibril_mutex_unlock(&instance->guard);
		return ENOSPC;
	}

	/* Check for existence */
	const endpoint_t *endpoint =
	    find_locked(instance, ep->address, ep->endpoint, ep->direction);
	if (endpoint != NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EEXISTS;
	}
	list_t *list = get_list(instance, ep->address);
	list_append(&ep->link, list);

	instance->free_bw -= ep->bandwidth;
	fibril_mutex_unlock(&instance->guard);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int usb_endpoint_manager_unregister_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);

	fibril_mutex_lock(&instance->guard);
	endpoint_t *ep = find_locked(instance, address, endpoint, direction);
	if (ep != NULL) {
		list_remove(&ep->link);
		instance->free_bw += ep->bandwidth;
	}

	fibril_mutex_unlock(&instance->guard);
	endpoint_destroy(ep);
	return (ep != NULL) ? EOK : EINVAL;
}
/*----------------------------------------------------------------------------*/
endpoint_t * usb_endpoint_manager_get_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);

	fibril_mutex_lock(&instance->guard);
	endpoint_t *ep = find_locked(instance, address, endpoint, direction);
	fibril_mutex_unlock(&instance->guard);

	return ep;
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
void usb_endpoint_manager_reset_if_need(
    usb_endpoint_manager_t *instance, usb_target_t target, const uint8_t *data)
{
	assert(instance);
	if (!usb_target_is_valid(target)) {
		usb_log_error("Invalid data when checking for toggle reset.\n");
		return;
	}

	assert(data);
	switch (data[1])
	{
	case 0x01: /* Clear Feature -- resets only cleared ep */
		/* Recipient is endpoint, value is zero (ENDPOINT_STALL) */
		if (((data[0] & 0xf) == 1) && ((data[2] | data[3]) == 0)) {
			fibril_mutex_lock(&instance->guard);
			/* endpoint number is < 16, thus first byte is enough */
			list_foreach(*get_list(instance, target.address), it) {
				endpoint_t *ep = endpoint_get_instance(it);
				if ((ep->address == target.address)
				    && (ep->endpoint = data[4])) {
					endpoint_toggle_set(ep,0);
				}
			}
			fibril_mutex_unlock(&instance->guard);
		}
	break;

	case 0x9: /* Set Configuration */
	case 0x11: /* Set Interface */
		/* Recipient must be device, this resets all endpoints,
		 * In fact there should be no endpoints but EP 0 registered
		 * as different interfaces use different endpoints. */
		if ((data[0] & 0xf) == 0) {
			fibril_mutex_lock(&instance->guard);
			list_foreach(*get_list(instance, target.address), it) {
				endpoint_t *ep = endpoint_get_instance(it);
				if (ep->address == target.address) {
					endpoint_toggle_set(ep,0);
				}
			}
			fibril_mutex_unlock(&instance->guard);
		}
	break;
	}
}
