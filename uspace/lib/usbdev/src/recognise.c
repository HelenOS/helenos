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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Functions for recognition of attached devices.
 */

#include <usb/dev/pipes.h>
#include <usb/dev/recognise.h>
#include <usb/dev/request.h>
#include <usb/classes/classes.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>

/** Get integer part from BCD coded number. */
#define BCD_INT(a) (((unsigned int)(a)) / 256)
/** Get fraction part from BCD coded number (as an integer, no less). */
#define BCD_FRAC(a) (((unsigned int)(a)) % 256)

/** Format for BCD coded number to be used in printf. */
#define BCD_FMT "%x.%x"
/** Arguments to printf for BCD coded number. */
#define BCD_ARGS(a) BCD_INT((a)), BCD_FRAC((a))

/** Add formatted match id.
 *
 * @param matches List of match ids where to add to.
 * @param score Score of the match.
 * @param format Printf-like format
 * @return Error code.
 */
static errno_t usb_add_match_id(match_id_list_t *matches, int score,
    char *match_str)
{
	assert(matches);

	match_id_t *match_id = create_match_id();
	if (match_id == NULL) {
		return ENOMEM;
	}

	match_id->id = match_str;
	match_id->score = score;
	add_match_id(matches, match_id);

	return EOK;
}

/** Add match id to list or return with error code.
 *
 * @param match_ids List of match ids.
 * @param score Match id score.
 * @param format Format of the matching string
 * @param ... Arguments for the format.
 */
#define ADD_MATCHID_OR_RETURN(match_ids, score, format, ...) \
	do { \
		char *str = NULL; \
		int __rc = asprintf(&str, format, ##__VA_ARGS__); \
		if (__rc >= 0) { \
			errno_t __rc = usb_add_match_id((match_ids), (score), str); \
			if (__rc != EOK) { \
				free(str); \
				return __rc; \
			} \
		} else { \
			return ENOMEM; \
		} \
	} while (0)

/** Create device match ids based on its interface.
 *
 * @param[in] desc_device Device descriptor.
 * @param[in] desc_interface Interface descriptor.
 * @param[out] matches Initialized list of match ids.
 * @return Error code (the two mentioned are not the only ones).
 * @retval EINVAL Invalid input parameters (expects non NULL pointers).
 * @retval ENOENT Device class is not "use interface".
 */
errno_t usb_device_create_match_ids_from_interface(
    const usb_standard_device_descriptor_t *desc_device,
    const usb_standard_interface_descriptor_t *desc_interface,
    match_id_list_t *matches)
{
	if (desc_interface == NULL || matches == NULL) {
		return EINVAL;
	}

	if (desc_interface->interface_class == USB_CLASS_USE_INTERFACE) {
		return ENOENT;
	}

	const char *classname = usb_str_class(desc_interface->interface_class);
	assert(classname != NULL);

#define IFACE_PROTOCOL_FMT "interface&class=%s&subclass=0x%02x&protocol=0x%02x"
#define IFACE_PROTOCOL_ARGS classname, desc_interface->interface_subclass, \
    desc_interface->interface_protocol

#define IFACE_SUBCLASS_FMT "interface&class=%s&subclass=0x%02x"
#define IFACE_SUBCLASS_ARGS classname, desc_interface->interface_subclass

#define IFACE_CLASS_FMT "interface&class=%s"
#define IFACE_CLASS_ARGS classname

#define VENDOR_RELEASE_FMT "vendor=0x%04x&product=0x%04x&release=" BCD_FMT
#define VENDOR_RELEASE_ARGS desc_device->vendor_id, desc_device->product_id, \
    BCD_ARGS(desc_device->device_version)

#define VENDOR_PRODUCT_FMT "vendor=0x%04x&product=0x%04x"
#define VENDOR_PRODUCT_ARGS desc_device->vendor_id, desc_device->product_id

#define VENDOR_ONLY_FMT "vendor=0x%04x"
#define VENDOR_ONLY_ARGS desc_device->vendor_id

	/*
	 * If the vendor is specified, create match ids with vendor with
	 * higher score.
	 * Then the same ones without the vendor part.
	 */
	if ((desc_device != NULL) && (desc_device->vendor_id != 0)) {
		/* First, interface matches with device release number. */
		ADD_MATCHID_OR_RETURN(matches, 250,
		    "usb&" VENDOR_RELEASE_FMT "&" IFACE_PROTOCOL_FMT,
		    VENDOR_RELEASE_ARGS, IFACE_PROTOCOL_ARGS);
		ADD_MATCHID_OR_RETURN(matches, 240,
		    "usb&" VENDOR_RELEASE_FMT "&" IFACE_SUBCLASS_FMT,
		    VENDOR_RELEASE_ARGS, IFACE_SUBCLASS_ARGS);
		ADD_MATCHID_OR_RETURN(matches, 230,
		    "usb&" VENDOR_RELEASE_FMT "&" IFACE_CLASS_FMT,
		    VENDOR_RELEASE_ARGS, IFACE_CLASS_ARGS);

		/* Next, interface matches without release number. */
		ADD_MATCHID_OR_RETURN(matches, 220,
		    "usb&" VENDOR_PRODUCT_FMT "&" IFACE_PROTOCOL_FMT,
		    VENDOR_PRODUCT_ARGS, IFACE_PROTOCOL_ARGS);
		ADD_MATCHID_OR_RETURN(matches, 210,
		    "usb&" VENDOR_PRODUCT_FMT "&" IFACE_SUBCLASS_FMT,
		    VENDOR_PRODUCT_ARGS, IFACE_SUBCLASS_ARGS);
		ADD_MATCHID_OR_RETURN(matches, 200,
		    "usb&" VENDOR_PRODUCT_FMT "&" IFACE_CLASS_FMT,
		    VENDOR_PRODUCT_ARGS, IFACE_CLASS_ARGS);

		/* Finally, interface matches with only vendor. */
		ADD_MATCHID_OR_RETURN(matches, 190,
		    "usb&" VENDOR_ONLY_FMT "&" IFACE_PROTOCOL_FMT,
		    VENDOR_ONLY_ARGS, IFACE_PROTOCOL_ARGS);
		ADD_MATCHID_OR_RETURN(matches, 180,
		    "usb&" VENDOR_ONLY_FMT "&" IFACE_SUBCLASS_FMT,
		    VENDOR_ONLY_ARGS, IFACE_SUBCLASS_ARGS);
		ADD_MATCHID_OR_RETURN(matches, 170,
		    "usb&" VENDOR_ONLY_FMT "&" IFACE_CLASS_FMT,
		    VENDOR_ONLY_ARGS, IFACE_CLASS_ARGS);
	}

	/* Now, the same but without any vendor specification. */
	ADD_MATCHID_OR_RETURN(matches, 160,
	    "usb&" IFACE_PROTOCOL_FMT,
	    IFACE_PROTOCOL_ARGS);
	ADD_MATCHID_OR_RETURN(matches, 150,
	    "usb&" IFACE_SUBCLASS_FMT,
	    IFACE_SUBCLASS_ARGS);
	ADD_MATCHID_OR_RETURN(matches, 140,
	    "usb&" IFACE_CLASS_FMT,
	    IFACE_CLASS_ARGS);

#undef IFACE_PROTOCOL_FMT
#undef IFACE_PROTOCOL_ARGS
#undef IFACE_SUBCLASS_FMT
#undef IFACE_SUBCLASS_ARGS
#undef IFACE_CLASS_FMT
#undef IFACE_CLASS_ARGS
#undef VENDOR_RELEASE_FMT
#undef VENDOR_RELEASE_ARGS
#undef VENDOR_PRODUCT_FMT
#undef VENDOR_PRODUCT_ARGS
#undef VENDOR_ONLY_FMT
#undef VENDOR_ONLY_ARGS

	/* As a last resort, try fallback driver. */
	ADD_MATCHID_OR_RETURN(matches, 10, "usb&interface&fallback");

	return EOK;
}

/** Create DDF match ids from USB device descriptor.
 *
 * @param matches List of match ids to extend.
 * @param device_descriptor Device descriptor returned by given device.
 * @return Error code.
 */
errno_t usb_device_create_match_ids_from_device_descriptor(
    const usb_standard_device_descriptor_t *device_descriptor,
    match_id_list_t *matches)
{
	/*
	 * Unless the vendor id is 0, the pair idVendor-idProduct
	 * quite uniquely describes the device.
	 */
	if (device_descriptor->vendor_id != 0) {
		/* First, with release number. */
		ADD_MATCHID_OR_RETURN(matches, 100,
		    "usb&vendor=0x%04x&product=0x%04x&release=" BCD_FMT,
		    (int) device_descriptor->vendor_id,
		    (int) device_descriptor->product_id,
		    BCD_ARGS(device_descriptor->device_version));
		
		/* Next, without release number. */
		ADD_MATCHID_OR_RETURN(matches, 90,
		    "usb&vendor=0x%04x&product=0x%04x",
		    (int) device_descriptor->vendor_id,
		    (int) device_descriptor->product_id);
	}

	/* Class match id */
	ADD_MATCHID_OR_RETURN(matches, 50, "usb&class=%s",
	    usb_str_class(device_descriptor->device_class));
	
	/* As a last resort, try fallback driver. */
	ADD_MATCHID_OR_RETURN(matches, 10, "usb&fallback");

	return EOK;
}


/** Create match ids describing attached device.
 *
 * @warning The list of match ids @p matches may change even when
 * function exits with error.
 *
 * @param ctrl_pipe Control pipe to given device (session must be already
 *	started).
 * @param matches Initialized list of match ids.
 * @return Error code.
 */
errno_t usb_device_create_match_ids(usb_pipe_t *ctrl_pipe,
    match_id_list_t *matches)
{
	assert(ctrl_pipe);
	errno_t rc;
	/*
	 * Retrieve device descriptor and add matches from it.
	 */
	usb_standard_device_descriptor_t device_descriptor;

	rc = usb_request_get_device_descriptor(ctrl_pipe, &device_descriptor);
	if (rc != EOK) {
		return rc;
	}

	rc = usb_device_create_match_ids_from_device_descriptor(
	    &device_descriptor, matches);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

/**
 * @}
 */
