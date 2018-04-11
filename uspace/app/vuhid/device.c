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

/** @addtogroup usbvirthid
 * @{
 */
/**
 * @file
 * Virtual USB HID device.
 */
#include "virthid.h"
#include <errno.h>
#include <stdio.h>
#include <str.h>
#include <assert.h>
#include <usb/classes/classes.h>

static errno_t on_data_from_device(usbvirt_device_t *dev,
    usb_endpoint_t ep, usb_transfer_type_t tr_type,
    void *data, size_t data_size, size_t *actual_size)
{
	vuhid_data_t *vuhid = dev->device_data;
	vuhid_interface_t *iface = vuhid->in_endpoints_mapping[ep];
	if (iface == NULL) {
		return ESTALL;
	}

	if (iface->on_data_in == NULL) {
		return EBADCHECKSUM;
	}

	return iface->on_data_in(iface, data, data_size, actual_size);
}

static errno_t on_data_to_device(usbvirt_device_t *dev,
    usb_endpoint_t ep, usb_transfer_type_t tr_type,
    const void *data, size_t data_size)
{
	vuhid_data_t *vuhid = dev->device_data;
	vuhid_interface_t *iface = vuhid->out_endpoints_mapping[ep];
	if (iface == NULL) {
		return ESTALL;
	}

	if (iface->on_data_out == NULL) {
		return EBADCHECKSUM;
	}

	return iface->on_data_out(iface, data, data_size);
}

static errno_t interface_life_fibril(void *arg)
{
	vuhid_interface_t *iface = arg;
	vuhid_data_t *hid_data = iface->vuhid_data;

	if (iface->live != NULL) {
		iface->live(iface);
	}

	fibril_mutex_lock(&hid_data->iface_count_mutex);
	hid_data->iface_died_count++;
	fibril_condvar_broadcast(&hid_data->iface_count_cv);
	fibril_mutex_unlock(&hid_data->iface_count_mutex);

	return EOK;
}


static vuhid_interface_t *find_interface_by_id(vuhid_interface_t **ifaces,
    const char *id)
{
	if ((ifaces == NULL) || (id == NULL)) {
		return NULL;
	}
	while (*ifaces != NULL) {
		if (str_cmp((*ifaces)->id, id) == 0) {
			return *ifaces;
		}
		ifaces++;
	}

	return NULL;
}

errno_t add_interface_by_id(vuhid_interface_t **interfaces, const char *id,
    usbvirt_device_t *dev)
{
	vuhid_interface_t *iface = find_interface_by_id(interfaces, id);
	if (iface == NULL) {
		return ENOENT;
	}

	if ((iface->in_data_size == 0) && (iface->out_data_size == 0)) {
		return EEMPTY;
	}

	/* Already used interface. */
	if (iface->vuhid_data != NULL) {
		return EEXIST;
	}

	vuhid_data_t *hid_data = dev->device_data;

	/* Check that we have not run out of available endpoints. */
	if ((iface->in_data_size > 0) &&
	    (hid_data->in_endpoint_first_free >= VUHID_ENDPOINT_MAX)) {
		return ELIMIT;
	}
	if ((iface->out_data_size > 0) &&
	    (hid_data->out_endpoint_first_free >= VUHID_ENDPOINT_MAX)) {
		return ELIMIT;
	}

	if (dev->descriptors->configuration->descriptor->interface_count >=
	    VUHID_INTERFACE_MAX) {
		return ELIMIT;
	}

	/*
	 * How may descriptors we would add?
	 */
	/* Start with interface descriptor. */
	size_t new_descriptor_count = 1;
	/* Having in/out data size positive means we would need endpoint. */
	if (iface->in_data_size > 0) {
		new_descriptor_count++;
	}
	if (iface->out_data_size > 0) {
		new_descriptor_count++;
	}
	/* Having report descriptor means adding the HID descriptor. */
	if (iface->report_descriptor != NULL) {
		new_descriptor_count++;
	}

	/* From now on, in case of errors, we goto to error_leave */
	errno_t rc;

	/*
	 * Prepare new descriptors.
	 */
	size_t descr_count = 0;
	size_t total_descr_size = 0;
	usb_standard_interface_descriptor_t *descr_iface = NULL;
	hid_descriptor_t *descr_hid = NULL;
	usb_standard_endpoint_descriptor_t *descr_ep_in = NULL;
	usb_standard_endpoint_descriptor_t *descr_ep_out = NULL;

	size_t ep_count = 0;
	if (iface->in_data_size > 0) {
		ep_count++;
	}
	if (iface->out_data_size > 0) {
		ep_count++;
	}
	assert(ep_count > 0);

	/* Interface would be needed always. */
	descr_iface = malloc(sizeof(usb_standard_interface_descriptor_t));
	if (descr_iface == NULL) {
		rc = ENOMEM;
		goto error_leave;
	}
	descr_count++;
	total_descr_size += sizeof(usb_standard_interface_descriptor_t);
	descr_iface->length = sizeof(usb_standard_interface_descriptor_t);
	descr_iface->descriptor_type = USB_DESCTYPE_INTERFACE;
	descr_iface->interface_number =
	    dev->descriptors->configuration->descriptor->interface_count;
	descr_iface->alternate_setting = 0;
	descr_iface->endpoint_count = ep_count;
	descr_iface->interface_class = USB_CLASS_HID;
	descr_iface->interface_subclass = iface->usb_subclass;
	descr_iface->interface_protocol = iface->usb_protocol;
	descr_iface->str_interface = 0;

	/* Create the HID descriptor. */
	if (iface->report_descriptor != NULL) {
		descr_hid = malloc(sizeof(hid_descriptor_t));
		if (descr_hid == NULL) {
			rc = ENOMEM;
			goto error_leave;
		}
		descr_count++;
		total_descr_size += sizeof(hid_descriptor_t);
		descr_hid->length = sizeof(hid_descriptor_t);
		descr_hid->type = USB_DESCTYPE_HID;
		descr_hid->hid_spec_release = 0x101;
		descr_hid->country_code = 0;
		descr_hid->descriptor_count = 1;
		descr_hid->descriptor1_type = USB_DESCTYPE_HID_REPORT;
		descr_hid->descriptor1_length = iface->report_descriptor_size;
	}

	/* Prepare the endpoint descriptors. */
	if (iface->in_data_size > 0) {
		descr_ep_in = malloc(sizeof(usb_standard_endpoint_descriptor_t));
		if (descr_ep_in == NULL) {
			rc = ENOMEM;
			goto error_leave;
		}
		descr_count++;
		total_descr_size += sizeof(usb_standard_endpoint_descriptor_t);
		descr_ep_in->length = sizeof(usb_standard_endpoint_descriptor_t);
		descr_ep_in->descriptor_type = USB_DESCTYPE_ENDPOINT;
		descr_ep_in->endpoint_address =
		    0x80 | hid_data->in_endpoint_first_free;
		descr_ep_in->attributes = 3;
		descr_ep_in->max_packet_size = iface->in_data_size;
		descr_ep_in->poll_interval = 10;
	}
	if (iface->out_data_size > 0) {
		descr_ep_out = malloc(sizeof(usb_standard_endpoint_descriptor_t));
		if (descr_ep_out == NULL) {
			rc = ENOMEM;
			goto error_leave;
		}
		descr_count++;
		total_descr_size += sizeof(usb_standard_endpoint_descriptor_t);
		descr_ep_out->length = sizeof(usb_standard_endpoint_descriptor_t);
		descr_ep_out->descriptor_type = USB_DESCTYPE_ENDPOINT;
		descr_ep_out->endpoint_address =
		    0x00 | hid_data->out_endpoint_first_free;
		descr_ep_out->attributes = 3;
		descr_ep_out->max_packet_size = iface->out_data_size;
		descr_ep_out->poll_interval = 10;
	}

	/* Extend existing extra descriptors with these ones. */
	usbvirt_device_configuration_extras_t *extra_descriptors;
	extra_descriptors = realloc(dev->descriptors->configuration->extra,
	    sizeof(usbvirt_device_configuration_extras_t) *
	    (dev->descriptors->configuration->extra_count + descr_count));
	if (extra_descriptors == NULL) {
		rc = ENOMEM;
		goto error_leave;
	}

	/* Launch the "life" fibril. */
	iface->vuhid_data = hid_data;
	fid_t life_fibril = fibril_create(interface_life_fibril, iface);
	if (life_fibril == 0) {
		rc = ENOMEM;
		goto error_leave;
	}

	/* Allocation is okay, we can (actually have to now) overwrite the
	 * original pointer.
	 */
	dev->descriptors->configuration->extra = extra_descriptors;
	extra_descriptors += dev->descriptors->configuration->extra_count;

	/* Initialize them. */
#define _APPEND_DESCRIPTOR(descriptor) \
	do { \
		if ((descriptor) != NULL) { \
			extra_descriptors->data = (uint8_t *) (descriptor); \
			extra_descriptors->length = (descriptor)->length; \
			extra_descriptors++; \
		} \
	} while (0)

	_APPEND_DESCRIPTOR(descr_iface);
	_APPEND_DESCRIPTOR(descr_hid);
	_APPEND_DESCRIPTOR(descr_ep_in);
	_APPEND_DESCRIPTOR(descr_ep_out);
#undef _APPEND_DESCRIPTOR
	dev->descriptors->configuration->extra_count += descr_count;

	/*
	 * Final changes.
	 * Increase the necessary counters, make endpoint mapping.
	 *
	 */
	if (iface->in_data_size > 0) {
		hid_data->in_endpoints_mapping[hid_data->in_endpoint_first_free] =
		    iface;
		dev->ops->data_in[hid_data->in_endpoint_first_free] =
		    on_data_from_device;
		hid_data->in_endpoint_first_free++;
	}
	if (iface->out_data_size > 0) {
		hid_data->out_endpoints_mapping[hid_data->out_endpoint_first_free] =
		    iface;
		dev->ops->data_out[hid_data->out_endpoint_first_free] =
		    on_data_to_device;
		hid_data->out_endpoint_first_free++;
	}

	hid_data->interface_mapping[dev->descriptors->configuration->descriptor->interface_count] =
	    iface;

	dev->descriptors->configuration->descriptor->interface_count++;
	dev->descriptors->configuration->descriptor->total_length +=
	    total_descr_size;

	hid_data->iface_count++;
	fibril_add_ready(life_fibril);

	return EOK;

error_leave:
	if (descr_iface != NULL) {
		free(descr_iface);
	}
	if (descr_hid != NULL) {
		free(descr_hid);
	}
	if (descr_ep_in != NULL) {
		free(descr_ep_in);
	}
	if (descr_ep_out != NULL) {
		free(descr_ep_out);
	}

	return rc;
}

void wait_for_interfaces_death(usbvirt_device_t *dev)
{
	vuhid_data_t *hid_data = dev->device_data;

	fibril_mutex_lock(&hid_data->iface_count_mutex);
	while (hid_data->iface_died_count < hid_data->iface_count) {
		fibril_condvar_wait(&hid_data->iface_count_cv,
		    &hid_data->iface_count_mutex);
	}
	fibril_mutex_unlock(&hid_data->iface_count_mutex);
}

/** @}
 */
