/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief Functions for recognising kind of attached devices.
 */
#include <usb/usbdrv.h>
#include <usb/classes/classes.h>
#include <stdio.h>
#include <errno.h>


#define BCD_INT(a) (((unsigned int)(a)) / 256)
#define BCD_FRAC(a) (((unsigned int)(a)) % 256)

#define BCD_FMT "%x.%x"
#define BCD_ARGS(a) BCD_INT((a)), BCD_FRAC((a))

/* FIXME: make this dynamic */
#define MATCH_STRING_MAX 256

/** Add formatted match id.
 *
 * @param matches List of match ids where to add to.
 * @param score Score of the match.
 * @param format Printf-like format
 * @return Error code.
 */
static int usb_add_match_id(match_id_list_t *matches, int score,
    const char *format, ...)
{
	char *match_str = NULL;
	match_id_t *match_id = NULL;
	int rc;
	
	match_str = malloc(MATCH_STRING_MAX + 1);
	if (match_str == NULL) {
		rc = ENOMEM;
		goto failure;
	}

	/*
	 * FIXME: replace with dynamic allocation of exact size
	 */
	va_list args;
	va_start(args, format	);
	vsnprintf(match_str, MATCH_STRING_MAX, format, args);
	match_str[MATCH_STRING_MAX] = 0;
	va_end(args);

	match_id = create_match_id();
	if (match_id == NULL) {
		rc = ENOMEM;
		goto failure;
	}

	match_id->id = match_str;
	match_id->score = score;
	add_match_id(matches, match_id);

	return EOK;
	
failure:
	if (match_str != NULL) {
		free(match_str);
	}
	if (match_id != NULL) {
		match_id->id = NULL;
		delete_match_id(match_id);
	}
	
	return rc;
}

/** Create match ids describing attached device.
 *
 * @warning The list of match ids @p matches may change even when
 * function exits with error.
 *
 * @param hc Open phone to host controller.
 * @param matches Initialized list of match ids.
 * @param address USB address of the attached device.
 * @return Error code.
 */
int usb_drv_create_device_match_ids(int hc, match_id_list_t *matches,
    usb_address_t address)
{
	int rc;
	usb_standard_device_descriptor_t device_descriptor;

	rc = usb_drv_req_get_device_descriptor(hc, address,
	    &device_descriptor);
	if (rc != EOK) {
		return rc;
	}

	/*
	 * Unless the vendor id is 0, the pair idVendor-idProduct
	 * quite uniquely describes the device.
	 */
	if (device_descriptor.vendor_id != 0) {
		/* First, with release number. */
		rc = usb_add_match_id(matches, 100,
		    "usb&vendor=%d&product=%d&release=" BCD_FMT,
		    (int) device_descriptor.vendor_id,
		    (int) device_descriptor.product_id,
		    BCD_ARGS(device_descriptor.device_version));
		if (rc != EOK) {
			return rc;
		}
		
		/* Next, without release number. */
		rc = usb_add_match_id(matches, 90, "usb&vendor=%d&product=%d",
		    (int) device_descriptor.vendor_id,
		    (int) device_descriptor.product_id);
		if (rc != EOK) {
			return rc;
		}

	}	

	/*
	 * If the device class does not point to interface,
	 * it is added immediatelly, otherwise full configuration
	 * descriptor must be obtained and parsed.
	 */
	if (device_descriptor.device_class != USB_CLASS_USE_INTERFACE) {
		rc = usb_add_match_id(matches, 50, "usb&class=%s",
		    usb_str_class(device_descriptor.device_class));
		if (rc != EOK) {
			return rc;
		}
	} else {
		/* TODO */
	}

	/*
	 * As a fallback, provide the simplest match id possible.
	 */
	rc = usb_add_match_id(matches, 1, "usb&fallback");
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

#if 0
/** Probe for device kind and register it in devman.
 *
 * @param hc Open phone to the host controller.
 * @param dev Parent device.
 * @param address Address of the (unknown) attached device.
 * @return Error code.
 */
int usb_drv_register_child_in_devman(int hc, device_t *dev,
    usb_address_t address)
{

}
#endif

/**
 * @}
 */
