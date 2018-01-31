/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2013 Jan Vesely
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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device driver framework.
 */

#ifndef LIBUSBDEV_ALTERNATE_IFACES_H_
#define LIBUSBDEV_ALTERNATE_IFACES_H_

#include <errno.h>
#include <usb/descriptor.h>
#include <stddef.h>
#include <stdint.h>

/** Wrapper for data related to alternate interface setting.
 * The pointers will typically point inside configuration descriptor and
 * thus you shall not deallocate them.
 */
typedef struct {
	/** Interface descriptor. */
	const usb_standard_interface_descriptor_t *interface;
	/** Pointer to start of descriptor tree bound with this interface. */
	const uint8_t *nested_descriptors;
	/** Size of data pointed by nested_descriptors in bytes. */
	size_t nested_descriptors_size;
} usb_alternate_interface_descriptors_t;

/** Alternate interface settings. */
typedef struct {
	/** Array of alternate interfaces descriptions. */
	usb_alternate_interface_descriptors_t *alternatives;
	/** Size of @c alternatives array. */
	size_t alternative_count;
	/** Index of currently selected one. */
	size_t current;
} usb_alternate_interfaces_t;

size_t usb_interface_count_alternates(const uint8_t *, size_t, uint8_t);
errno_t usb_alternate_interfaces_init(usb_alternate_interfaces_t *,
    const uint8_t *, size_t, int);
void usb_alternate_interfaces_deinit(usb_alternate_interfaces_t *);

#endif
/**
 * @}
 */
