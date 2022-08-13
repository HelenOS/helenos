/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <usb/dev.h>
#include <errno.h>
#include <usb_iface.h>
#include <str.h>
#include <stdio.h>

/** Resolve handle and address of USB device from its path.
 *
 * This is a wrapper working on best effort principle.
 * If the resolving fails, if will not give much details about what
 * is wrong.
 * Typically, error from this function would be reported to the user
 * as "bad device specification" or "device does not exist".
 *
 * The path can be specified in following format:
 *  - devman path (e.g. /hw/pci0/.../usb01_a5
 *  - bus number and device address (e.g. 5.1)
 *  - bus number, device address and device function (e.g. 2.1/HID0/keyboard)
 *
 * @param[in] dev_path Path to the device.
 * @param[out] out_hc_handle Where to store handle of a parent host controller.
 * @param[out] out_dev_addr Where to store device (USB) address.
 * @param[out] out_dev_handle Where to store device handle.
 * @return Error code.
 */
errno_t usb_resolve_device_handle(const char *dev_path, devman_handle_t *dev_handle)
{
	if (dev_path == NULL || dev_handle == NULL) {
		return EBADMEM;
	}

	/* First, try to get the device handle. */
	errno_t rc = devman_fun_get_handle(dev_path, dev_handle, 0);

	/* Next, try parsing dev_handle from the provided string */
	if (rc != EOK) {
		*dev_handle = strtoul(dev_path, NULL, 10);
		//FIXME: check errno
		rc = EOK;
	}
	return rc;
}
