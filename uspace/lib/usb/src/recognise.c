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

/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief Functions for recognising kind of attached devices.
 */
#include <sys/types.h>
#include <usb/pipes.h>
#include <usb/recognise.h>
#include <usb/ddfiface.h>
#include <usb/request.h>
#include <usb/classes/classes.h>
#include <stdio.h>
#include <errno.h>

static size_t device_name_index = 0;
static FIBRIL_MUTEX_INITIALIZE(device_name_index_mutex);

device_ops_t child_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface_hub_child_impl
};

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

#define ADD_MATCHID_OR_RETURN(match_ids, score, format, ...) \
	do { \
		int __rc = usb_add_match_id((match_ids), (score), \
		    format, ##__VA_ARGS__); \
		if (__rc != EOK) { \
			return __rc; \
		} \
	} while (0)

/** Create device match ids based on its interface.
 *
 * @param[in] descriptor Interface descriptor.
 * @param[out] matches Initialized list of match ids.
 * @return Error code (the two mentioned are not the only ones).
 * @retval EINVAL Invalid input parameters (expects non NULL pointers).
 * @retval ENOENT Interface does not specify class.
 */
int usb_device_create_match_ids_from_interface(
    const usb_standard_device_descriptor_t *desc_device,
    const usb_standard_interface_descriptor_t *desc_interface,
    match_id_list_t *matches)
{
	if (desc_interface == NULL) {
		return EINVAL;
	}
	if (matches == NULL) {
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

	return EOK;
}

/** Create DDF match ids from USB device descriptor.
 *
 * @param matches List of match ids to extend.
 * @param device_descriptor Device descriptor returned by given device.
 * @return Error code.
 */
int usb_device_create_match_ids_from_device_descriptor(
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

	/*
	 * If the device class points to interface we skip adding
	 * class directly but we add a multi interface device.
	 */
	if (device_descriptor->device_class != USB_CLASS_USE_INTERFACE) {
		ADD_MATCHID_OR_RETURN(matches, 50, "usb&class=%s",
		    usb_str_class(device_descriptor->device_class));
	} else {
		ADD_MATCHID_OR_RETURN(matches, 50, "usb&mid");
	}
	
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
int usb_device_create_match_ids(usb_endpoint_pipe_t *ctrl_pipe,
    match_id_list_t *matches)
{
	int rc;
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

	/*
	 * As a fallback, provide the simplest match id possible.
	 */
	ADD_MATCHID_OR_RETURN(matches, 1, "usb&fallback");

	return EOK;
}

/** Probe for device kind and register it in devman.
 *
 * @param[in] address Address of the (unknown) attached device.
 * @param[in] hc_handle Handle of the host controller.
 * @param[in] parent Parent device.
 * @param[out] child_handle Handle of the child device.
 * @return Error code.
 */
int usb_device_register_child_in_devman(usb_address_t address,
    devman_handle_t hc_handle,
    device_t *parent, devman_handle_t *child_handle)
{
	size_t this_device_name_index;

	fibril_mutex_lock(&device_name_index_mutex);
	this_device_name_index = device_name_index;
	device_name_index++;
	fibril_mutex_unlock(&device_name_index_mutex);

	device_t *child = NULL;
	char *child_name = NULL;
	int rc;
	usb_device_connection_t dev_connection;
	usb_endpoint_pipe_t ctrl_pipe;

	rc = usb_device_connection_initialize(&dev_connection, hc_handle, address);
	if (rc != EOK) {
		goto failure;
	}

	rc = usb_endpoint_pipe_initialize_default_control(&ctrl_pipe,
	    &dev_connection);
	if (rc != EOK) {
		goto failure;
	}

	child = create_device();
	if (child == NULL) {
		rc = ENOMEM;
		goto failure;
	}

	/*
	 * TODO: Once the device driver framework support persistent
	 * naming etc., something more descriptive could be created.
	 */
	rc = asprintf(&child_name, "usbdev%02zu", this_device_name_index);
	if (rc < 0) {
		goto failure;
	}
	child->parent = parent;
	child->name = child_name;
	child->ops = &child_ops;

	rc = usb_endpoint_pipe_start_session(&ctrl_pipe);
	if (rc != EOK) {
		goto failure;
	}

	rc = usb_device_create_match_ids(&ctrl_pipe, &child->match_ids);
	if (rc != EOK) {
		goto failure;
	}

	rc = usb_endpoint_pipe_end_session(&ctrl_pipe);
	if (rc != EOK) {
		goto failure;
	}

	rc = child_device_register(child, parent);
	if (rc != EOK) {
		goto failure;
	}

	if (child_handle != NULL) {
		*child_handle = child->handle;
	}

	return EOK;

failure:
	if (child != NULL) {
		child->name = NULL;
		/* This takes care of match_id deallocation as well. */
		delete_device(child);
	}
	if (child_name != NULL) {
		free(child_name);
	}

	return rc;
}


/**
 * @}
 */
