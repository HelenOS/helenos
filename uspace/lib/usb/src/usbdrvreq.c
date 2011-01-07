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
 * @brief USB driver - standard USB requests (implementation).
 */
#include <usb/usbdrv.h>
#include <errno.h>

/** Change address of connected device.
 *
 * @see usb_drv_reserve_default_address
 * @see usb_drv_release_default_address
 * @see usb_drv_request_address
 * @see usb_drv_release_address
 * @see usb_drv_bind_address
 *
 * @param phone Open phone to HC driver.
 * @param old_address Current address.
 * @param address Address to be set.
 * @return Error code.
 */
int usb_drv_req_set_address(int phone, usb_address_t old_address,
    usb_address_t new_address)
{
	/* Prepare the target. */
	usb_target_t target = {
		.address = old_address,
		.endpoint = 0
	};

	/* Prepare the setup packet. */
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 0,
		.request = USB_DEVREQ_SET_ADDRESS,
		.index = 0,
		.length = 0,
	};
	setup_packet.value = new_address;

	int rc = usb_drv_psync_control_write(phone, target,
	    &setup_packet, sizeof(setup_packet), NULL, 0);

	return rc;
}

/** Retrieve USB descriptor of connected USB device.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device USB address.
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] langauge Language index.
 * @param[out] buffer Buffer where to store the retrieved descriptor.
 * @param[in] size Size of the @p buffer.
 * @param[out] actual_size Number of bytes actually transferred.
 * @return Error code.
 */
int usb_drv_req_get_descriptor(int hc_phone, usb_address_t address,
    usb_request_type_t request_type,
    uint8_t descriptor_type, uint8_t descriptor_index,
    uint16_t language,
    void *buffer, size_t size, size_t *actual_size)
{
	/* Prepare the target. */
	usb_target_t target = {
		.address = address,
		.endpoint = 0
	};

	/* Prepare the setup packet. */
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 128 | (request_type << 5),
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.index = language,
		.length = (uint16_t) size,
	};
	setup_packet.value_high = descriptor_type;
	setup_packet.value_low = descriptor_index;
	
	/* Perform CONTROL READ */
	int rc = usb_drv_psync_control_read(hc_phone, target,
	    &setup_packet, sizeof(setup_packet),
	    buffer, size, actual_size);
	
	return rc;
}

/** Retrieve device descriptor of connected USB device.
 *
 * @param[in] phone Open phone to HC driver.
 * @param[in] address Device USB address.
 * @param[out] descriptor Storage for the device descriptor.
 * @return Error code.
 * @retval EBADMEM @p descriptor is NULL.
 */
int usb_drv_req_get_device_descriptor(int phone, usb_address_t address,
    usb_standard_device_descriptor_t *descriptor)
{
	if (descriptor == NULL) {
		return EBADMEM;
	}

	/* Prepare the target. */
	usb_target_t target = {
		.address = address,
		.endpoint = 0
	};

	/* Prepare the setup packet. */
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 128,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.index = 0,
		.length = sizeof(usb_standard_device_descriptor_t)
	};
	setup_packet.value_high = USB_DESCTYPE_DEVICE;
	setup_packet.value_low = 0;

	/* Prepare local descriptor. */
	size_t actually_transferred = 0;
	usb_standard_device_descriptor_t descriptor_tmp;

	/* Perform the control read transaction. */
	int rc = usb_drv_psync_control_read(phone, target,
	    &setup_packet, sizeof(setup_packet),
	    &descriptor_tmp, sizeof(descriptor_tmp), &actually_transferred);

	if (rc != EOK) {
		return rc;
	}

	/* Verify that all data has been transferred. */
	if (actually_transferred < sizeof(descriptor_tmp)) {
		return ELIMIT;
	}

	/* Everything is okay, copy the descriptor. */
	memcpy(descriptor, &descriptor_tmp,
	    sizeof(descriptor_tmp));

	return EOK;
}


/** Retrieve configuration descriptor of connected USB device.
 *
 * The function does not retrieve additional data binded with configuration
 * descriptor (such as its interface and endpoint descriptors) - use
 * usb_drv_req_get_full_configuration_descriptor() instead.
 *
 * @param[in] phone Open phone to HC driver.
 * @param[in] address Device USB address.
 * @param[in] index Configuration descriptor index.
 * @param[out] descriptor Storage for the configuration descriptor.
 * @return Error code.
 * @retval EBADMEM @p descriptor is NULL.
 */
int usb_drv_req_get_bare_configuration_descriptor(int phone,
    usb_address_t address, int index,
    usb_standard_configuration_descriptor_t *descriptor)
{
	if (descriptor == NULL) {
		return EBADMEM;
	}

	/* Prepare the target. */
	usb_target_t target = {
		.address = address,
		.endpoint = 0
	};

	/* Prepare the setup packet. */
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 128,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.index = 0,
		.length = sizeof(usb_standard_configuration_descriptor_t)
	};
	setup_packet.value_high = USB_DESCTYPE_CONFIGURATION;
	setup_packet.value_low = index;

	/* Prepare local descriptor. */
	size_t actually_transferred = 0;
	usb_standard_configuration_descriptor_t descriptor_tmp;

	/* Perform the control read transaction. */
	int rc = usb_drv_psync_control_read(phone, target,
	    &setup_packet, sizeof(setup_packet),
	    &descriptor_tmp, sizeof(descriptor_tmp), &actually_transferred);

	if (rc != EOK) {
		return rc;
	}

	/* Verify that all data has been transferred. */
	if (actually_transferred < sizeof(descriptor_tmp)) {
		return ELIMIT;
	}

	/* Everything is okay, copy the descriptor. */
	memcpy(descriptor, &descriptor_tmp,
	    sizeof(descriptor_tmp));

	return EOK;
}

/** Retrieve full configuration descriptor of connected USB device.
 *
 * @warning The @p buffer might be touched (i.e. its contents changed)
 * even when error occurres.
 *
 * @param[in] phone Open phone to HC driver.
 * @param[in] address Device USB address.
 * @param[in] index Configuration descriptor index.
 * @param[out] buffer Buffer for the whole configuration descriptor.
 * @param[in] buffer_size Size of the prepared @p buffer.
 * @param[out] actual_buffer_size Bytes actually transfered.
 * @return Error code.
 * @retval EBADMEM @p descriptor is NULL.
 */
int usb_drv_req_get_full_configuration_descriptor(int phone,
    usb_address_t address, int index,
    void *buffer, size_t buffer_size, size_t *actual_buffer_size)
{
	if (buffer == NULL) {
		return EBADMEM;
	}

	/* Prepare the target. */
	usb_target_t target = {
		.address = address,
		.endpoint = 0
	};

	/* Prepare the setup packet. */
	usb_device_request_setup_packet_t setup_packet = {
		.request_type = 128,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.index = 0,
		.length = buffer_size
	};
	setup_packet.value_high = USB_DESCTYPE_CONFIGURATION;
	setup_packet.value_low = index;

	/* Perform the control read transaction. */
	int rc = usb_drv_psync_control_read(phone, target,
	    &setup_packet, sizeof(setup_packet),
	    buffer, buffer_size, actual_buffer_size);

	return rc;
}


/**
 * @}
 */
