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
#include <usb_iface.h>
#include <usb/usbdrv.h>
#include <usb/pipes.h>
#include <usb/recognise.h>
#include <usb/request.h>
#include <usb/classes/classes.h>
#include <stdio.h>
#include <errno.h>

static size_t device_name_index = 0;
static FIBRIL_MUTEX_INITIALIZE(device_name_index_mutex);

/** Callback for getting host controller handle.
 *
 * @param dev Device in question.
 * @param[out] handle Devman handle of the host controller.
 * @return Error code.
 */
static int usb_iface_get_hc_handle(device_t *dev, devman_handle_t *handle)
{
	assert(dev);
	assert(dev->parent != NULL);

	device_t *parent = dev->parent;

	if (parent->ops && parent->ops->interfaces[USB_DEV_IFACE]) {
		usb_iface_t *usb_iface
		    = (usb_iface_t *) parent->ops->interfaces[USB_DEV_IFACE];
		assert(usb_iface != NULL);
		if (usb_iface->get_hc_handle) {
			int rc = usb_iface->get_hc_handle(parent, handle);
			return rc;
		}
	}

	return ENOTSUP;
}

static usb_iface_t usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle
};

device_ops_t child_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface
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

/** Create DDF match ids from USB device descriptor.
 *
 * @param matches List of match ids to extend.
 * @param device_descriptor Device descriptor returned by given device.
 * @return Error code.
 */
int usb_drv_create_match_ids_from_device_descriptor(
    match_id_list_t *matches,
    const usb_standard_device_descriptor_t *device_descriptor)
{
	int rc;
	
	/*
	 * Unless the vendor id is 0, the pair idVendor-idProduct
	 * quite uniquely describes the device.
	 */
	if (device_descriptor->vendor_id != 0) {
		/* First, with release number. */
		rc = usb_add_match_id(matches, 100,
		    "usb&vendor=0x%04x&product=0x%04x&release=" BCD_FMT,
		    (int) device_descriptor->vendor_id,
		    (int) device_descriptor->product_id,
		    BCD_ARGS(device_descriptor->device_version));
		if (rc != EOK) {
			return rc;
		}
		
		/* Next, without release number. */
		rc = usb_add_match_id(matches, 90,
		    "usb&vendor=0x%04x&product=0x%04x",
		    (int) device_descriptor->vendor_id,
		    (int) device_descriptor->product_id);
		if (rc != EOK) {
			return rc;
		}
	}	

	/*
	 * If the device class points to interface we skip adding
	 * class directly.
	 */
	if (device_descriptor->device_class != USB_CLASS_USE_INTERFACE) {
		rc = usb_add_match_id(matches, 50, "usb&class=%s",
		    usb_str_class(device_descriptor->device_class));
		if (rc != EOK) {
			return rc;
		}
	}
	
	return EOK;
}

/** Create DDF match ids from USB configuration descriptor.
 * The configuration descriptor is expected to be in the complete form,
 * i.e. including interface, endpoint etc. descriptors.
 *
 * @param matches List of match ids to extend.
 * @param config_descriptor Configuration descriptor returned by given device.
 * @param total_size Size of the @p config_descriptor.
 * @return Error code.
 */
int usb_drv_create_match_ids_from_configuration_descriptor(
    match_id_list_t *matches,
    const void *config_descriptor, size_t total_size)
{
	/*
	 * Iterate through config descriptor to find the interface
	 * descriptors.
	 */
	size_t position = sizeof(usb_standard_configuration_descriptor_t);
	while (position + 1 < total_size) {
		uint8_t *current_descriptor
		    = ((uint8_t *) config_descriptor) + position;
		uint8_t cur_descr_len = current_descriptor[0];
		uint8_t cur_descr_type = current_descriptor[1];

		if (cur_descr_len == 0) {
			return ENOENT;
		}
		
		position += cur_descr_len;
		
		if (cur_descr_type != USB_DESCTYPE_INTERFACE) {
			continue;
		}
		
		/*
		 * Finally, we found an interface descriptor.
		 */
		usb_standard_interface_descriptor_t *interface
		    = (usb_standard_interface_descriptor_t *)
		    current_descriptor;
		
		int rc = usb_add_match_id(matches, 50,
		    "usb&interface&class=%s",
		    usb_str_class(interface->interface_class));
		if (rc != EOK) {
			return rc;
		}
	}
	
	return EOK;
}

/** Add match ids based on configuration descriptor.
 *
 * @param pipe Control pipe to the device.
 * @param matches Match ids list to add matches to.
 * @param config_count Number of configurations the device has.
 * @return Error code.
 */
static int usb_add_config_descriptor_match_ids(usb_endpoint_pipe_t *pipe,
    match_id_list_t *matches, int config_count)
{
	int final_rc = EOK;
	
	int config_index;
	for (config_index = 0; config_index < config_count; config_index++) {
		int rc;
		usb_standard_configuration_descriptor_t config_descriptor;
		rc = usb_request_get_bare_configuration_descriptor(pipe,
		    config_index, &config_descriptor);
		if (rc != EOK) {
			final_rc = rc;
			continue;
		}

		size_t full_config_descriptor_size;
		void *full_config_descriptor
		    = malloc(config_descriptor.total_length);
		rc = usb_request_get_full_configuration_descriptor(pipe,
		    config_index,
		    full_config_descriptor, config_descriptor.total_length,
		    &full_config_descriptor_size);
		if (rc != EOK) {
			final_rc = rc;
			continue;
		}
		if (full_config_descriptor_size
		    != config_descriptor.total_length) {
			final_rc = ERANGE;
			continue;
		}
		
		rc = usb_drv_create_match_ids_from_configuration_descriptor(
		    matches,
		    full_config_descriptor, full_config_descriptor_size);
		if (rc != EOK) {
			final_rc = rc;
			continue;
		}
		
	}
	
	return final_rc;
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

	rc = usb_drv_create_match_ids_from_device_descriptor(matches,
	    &device_descriptor);
	if (rc != EOK) {
		return rc;
	}

	/*
	 * Go through all configurations and add matches
	 * based on interface class.
	 */
	rc = usb_add_config_descriptor_match_ids(ctrl_pipe, matches,
	    device_descriptor.configuration_count);
	if (rc != EOK) {
		return rc;
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
