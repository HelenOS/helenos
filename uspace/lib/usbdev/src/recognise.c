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

#include <sys/types.h>
#include <fibril_synch.h>
#include <usb/debug.h>
#include <usb/dev/hub.h>
#include <usb/dev/pipes.h>
#include <usb/dev/recognise.h>
#include <usb/ddfiface.h>
#include <usb/dev/request.h>
#include <usb/classes/classes.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

/** DDF operations of child devices. */
static ddf_dev_ops_t child_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface_hub_child_impl
};

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
static int usb_add_match_id(match_id_list_t *matches, int score,
    const char *match_str)
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
		if (__rc > 0) { \
			__rc = usb_add_match_id((match_ids), (score), str); \
		} \
		if (__rc != EOK) { \
			free(str); \
			return __rc; \
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
int usb_device_create_match_ids_from_interface(
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
int usb_device_create_match_ids(usb_pipe_t *ctrl_pipe,
    match_id_list_t *matches)
{
	assert(ctrl_pipe);
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

	return EOK;
}

/** Probe for device kind and register it in devman.
 *
 * @param[in] ctrl_pipe Control pipe to the device.
 * @param[in] parent Parent device.
 * @param[in] dev_ops Child device ops. Default child_ops will be used if NULL.
 * @param[in] dev_data Arbitrary pointer to be stored in the child
 *	as @c driver_data.
 * @param[out] child_fun Storage where pointer to allocated child function
 *	will be written.
 * @return Error code.
 *
 */
int usb_device_register_child_in_devman(usb_pipe_t *ctrl_pipe,
    ddf_dev_t *parent, ddf_fun_t *fun, ddf_dev_ops_t *dev_ops)
{
	if (ctrl_pipe == NULL)
		return EINVAL;
	
	if (!dev_ops && ddf_fun_data_get(fun) != NULL) {
		usb_log_warning("Using standard fun ops with arbitrary "
		    "driver data. This does not have to work.\n");
	}
	
	/** Index to append after device name for uniqueness. */
	static atomic_t device_name_index = {0};
	const size_t this_device_name_index =
	    (size_t) atomic_preinc(&device_name_index);
	
	int rc;
	
	/*
	 * TODO: Once the device driver framework support persistent
	 * naming etc., something more descriptive could be created.
	 */
	char child_name[12];  /* The format is: "usbAB_aXYZ", length 11 */
	rc = snprintf(child_name, sizeof(child_name),
	    "usb%02zu_a%d", this_device_name_index, ctrl_pipe->wire->address);
	if (rc < 0) {
		goto failure;
	}
	
	rc = ddf_fun_set_name(fun, child_name);
	if (rc != EOK)
		goto failure;
	
	if (dev_ops != NULL)
		ddf_fun_set_ops(fun, dev_ops);
	else
		ddf_fun_set_ops(fun, &child_ops);
	
	/*
	 * Store the attached device in fun
	 * driver data if there is no other data
	 */
	if (ddf_fun_data_get(fun) == NULL) {
		usb_hub_attached_device_t *new_device = ddf_fun_data_alloc(
		    fun, sizeof(usb_hub_attached_device_t));
		if (!new_device) {
			rc = ENOMEM;
			goto failure;
		}
		
		new_device->address = ctrl_pipe->wire->address;
		new_device->fun = fun;
	}
	
	match_id_list_t match_ids;
	init_match_ids(&match_ids);
	rc = usb_device_create_match_ids(ctrl_pipe, &match_ids);
	if (rc != EOK)
		goto failure;
	
	list_foreach(match_ids.ids, link, match_id_t, match_id) {
		rc = ddf_fun_add_match_id(fun, match_id->id, match_id->score);
		if (rc != EOK) {
			clean_match_ids(&match_ids);
			goto failure;
		}
	}
	
	clean_match_ids(&match_ids);
	
	rc = ddf_fun_bind(fun);
	if (rc != EOK)
		goto failure;
	
	return EOK;
	
failure:
	return rc;
}

/**
 * @}
 */
