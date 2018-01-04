/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2011 Vojtech Horky
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
