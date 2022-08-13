/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
