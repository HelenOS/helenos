/*
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

/** @addtogroup libusb
 * @{
 */
/**
 * @file
 * Host controller common functions (implementation).
 */
#include <stdio.h>
#include <str_error.h>
#include <errno.h>
#include <assert.h>
#include <bool.h>
#include <usb/host.h>
#include <usb/descriptor.h>
#include <devman.h>

/** Get host controller handle by its class index.
 *
 * @param class_index Class index for the host controller.
 * @param hc_handle Where to store the HC handle
 *	(can be NULL for existence test only).
 * @return Error code.
 */
int usb_ddf_get_hc_handle_by_class(size_t class_index,
    devman_handle_t *hc_handle)
{
	char *class_index_str;
	devman_handle_t hc_handle_tmp;
	int rc;

	rc = asprintf(&class_index_str, "%zu", class_index);
	if (rc < 0) {
		return ENOMEM;
	}
	rc = devman_device_get_handle_by_class("usbhc", class_index_str,
	    &hc_handle_tmp, 0);
	free(class_index_str);
	if (rc != EOK) {
		return rc;
	}

	if (hc_handle != NULL) {
		*hc_handle = hc_handle_tmp;
	}

	return EOK;
}

/** @}
 */
