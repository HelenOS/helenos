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
 * Device keeper structure and functions.
 *
 * Typical USB host controller needs to keep track of various settings for
 * each device that is connected to it.
 * State of toggle bit, device speed etc. etc.
 * This structure shall simplify the management.
 */
#ifndef LIBUSB_HOST_BANDWIDTH_H
#define LIBUSB_HOST_BANDWIDTH_H

#include <adt/hash_table.h>
#include <fibril_synch.h>
#include <usb/usb.h>

#define BANDWIDTH_TOTAL_USB11 12000000
#define BANDWIDTH_AVAILABLE_USB11 ((BANDWIDTH_TOTAL_USB11 / 10) * 9)

typedef struct bandwidth {
	hash_table_t reserved;
	fibril_mutex_t guard;
	size_t free;
	size_t (*usage_fnc)(usb_speed_t, usb_transfer_type_t, size_t, size_t);
} bandwidth_t;

size_t bandwidth_count_usb11(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size);

int bandwidth_init(bandwidth_t *instance, size_t bandwidth,
    size_t (*usage_fnc)(usb_speed_t, usb_transfer_type_t, size_t, size_t));

void bandwidth_destroy(bandwidth_t *instance);

int bandwidth_reserve(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction, usb_speed_t speed,
    usb_transfer_type_t transfer_type, size_t max_packet_size, size_t size,
    unsigned interval);

int bandwidth_release(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction);

int bandwidth_use(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction);

int bandwidth_free(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction);

#endif
/**
 * @}
 */
