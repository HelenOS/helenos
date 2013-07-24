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
#include <usb/hc.h>
#include <errno.h>
#include <usb_iface.h>
#include <str.h>
#include <stdio.h>

#define MAX_DEVICE_PATH 1024

static bool try_parse_bus_and_address(const char *path,
    const char **func_start,
    devman_handle_t *out_hc_handle, usb_address_t *out_device_address)
{
	uint64_t sid;
	size_t address;
	int rc;
	const char *ptr;

	rc = str_uint64_t(path, &ptr, 10, false, &sid);
	if (rc != EOK) {
		return false;
	}
	if ((*ptr == ':') || (*ptr == '.')) {
		ptr++;
	} else {
		return false;
	}
	rc = str_size_t(ptr, func_start, 10, false, &address);
	if (rc != EOK) {
		return false;
	}
	rc = usb_ddf_get_hc_handle_by_sid(sid, out_hc_handle);
	if (rc != EOK) {
		return false;
	}
	if (out_device_address != NULL) {
		*out_device_address = (usb_address_t) address;
	}
	return true;
}

static int get_device_handle_by_address(devman_handle_t hc_handle, int addr,
    devman_handle_t *dev_handle)
{
	usb_hc_connection_t conn;
	usb_hc_connection_initialize(&conn, hc_handle);

	const int rc = usb_hc_get_handle_by_address(&conn, addr, dev_handle);

	return rc;
}

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
int usb_resolve_device_handle(const char *dev_path, devman_handle_t *dev_handle)
{
	if (dev_path == NULL || dev_handle == NULL) {
		return EBADMEM;
	}

	devman_handle_t hc;
	usb_address_t addr = -1;
	int rc;
	const char *func_start = NULL;
	char *path = NULL;

	/* First try the BUS.ADDR format. */
	if (try_parse_bus_and_address(dev_path, &func_start, &hc, &addr)) {
		/*
		 * Now get the handle of the device. We will need that
		 * in both cases. If there is only BUS.ADDR, it will
		 * be the handle to be returned to the caller, otherwise
		 * we will need it to resolve the path to which the
		 * suffix would be appended.
		 */
		/* If there is nothing behind the BUS.ADDR, we will
		 * get the device handle from the host controller.
		 * Otherwise, we will
		 */

		rc = get_device_handle_by_address(hc, addr, dev_handle);
		if (rc != EOK) {
			return rc;
		}
		if (str_length(func_start) > 0) {
			char tmp_path[MAX_DEVICE_PATH];
			rc = devman_fun_get_path(*dev_handle,
			    tmp_path, MAX_DEVICE_PATH);
			if (rc != EOK) {
				return rc;
			}
			rc = asprintf(&path, "%s%s", tmp_path, func_start);
			if (rc < 0) {
				return ENOMEM;
			}
		} else {
			/* Everything is resolved. Get out of here. */
			return EOK;
		}
	} else {
		path = str_dup(dev_path);
		if (path == NULL) {
			return ENOMEM;
		}
	}

	/* First try to get the device handle. */
	rc = devman_fun_get_handle(path, dev_handle, 0);
	if (rc != EOK) {
		free(path);
		/* Invalid path altogether. */
		return rc;
	}

	/* Remove suffixes and hope that we will encounter device node. */
	while (str_length(path) > 0) {
		/* Get device handle first. */
		devman_handle_t tmp_handle;
		rc = devman_fun_get_handle(path, &tmp_handle, 0);
		if (rc != EOK) {
			free(path);
			return rc;
		}

		/* Remove the last suffix. */
		char *slash_pos = str_rchr(path, '/');
		if (slash_pos != NULL) {
			*slash_pos = 0;
		}
	}

	free(path);


	return EOK;
}
