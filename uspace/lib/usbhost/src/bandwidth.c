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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * Bandwidth calculation functions. Shared among uhci, ohci and ehci drivers.
 */

#include <assert.h>
#include <stdlib.h>

#include "endpoint.h"
#include "bus.h"

#include "bandwidth.h"

/** Calculate bandwidth that needs to be reserved for communication with EP.
 * Calculation follows USB 1.1 specification.
 * @param ep Registered endpoint
 * @param size Number of bytes to transfer.
 * @param max_packet_size Maximum bytes in one packet.
 */
size_t bandwidth_count_usb11(endpoint_t *ep, size_t size)
{
	assert(ep);
	assert(ep->device);

	const usb_transfer_type_t type = ep->transfer_type;

	/* We care about bandwidth only for interrupt and isochronous. */
	if ((type != USB_TRANSFER_INTERRUPT)
	    && (type != USB_TRANSFER_ISOCHRONOUS)) {
		return 0;
	}

	const size_t max_packet_size = ep->max_packet_size;

	const unsigned packet_count =
	    (size + max_packet_size - 1) / max_packet_size;
	/* TODO: It may be that ISO and INT transfers use only one packet per
	 * transaction, but I did not find text in USB spec to confirm this */
	/* NOTE: All data packets will be considered to be max_packet_size */
	switch (ep->device->speed)
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
size_t bandwidth_count_usb20(endpoint_t *ep, size_t size)
{
	assert(ep);

	const usb_transfer_type_t type = ep->transfer_type;

	/* We care about bandwidth only for interrupt and isochronous. */
	if ((type != USB_TRANSFER_INTERRUPT)
	    && (type != USB_TRANSFER_ISOCHRONOUS)) {
		return 0;
	}
	//TODO Implement
	return 0;
}
