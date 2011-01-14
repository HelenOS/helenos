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

/**  Prepare USB target for control endpoint.
 *
 * @param name Variable name with the USB target.
 * @param target_address Target USB address.
 */
#define PREPARE_TARGET(name, target_address) \
	usb_target_t name = { \
		.address = target_address, \
		.endpoint = 0 \
	}

/** Prepare setup packet.
 *
 * @param name Variable name with the setup packet.
 * @param p_direction Data transfer direction.
 * @param p_type Request type (standard/class/vendor)
 * @param p_recipient Recipient of the request.
 * @param p_request Request.
 * @param p_value wValue field of setup packet.
 * @param p_index wIndex field of setup packet.
 * @param p_length Length of extra data.
 */
#define PREPARE_SETUP_PACKET(name, p_direction, p_type, p_recipient, \
    p_request, p_value, p_index, p_length) \
	usb_device_request_setup_packet_t name = { \
		.request_type = \
			((p_direction) == USB_DIRECTION_IN ? 128 : 0) \
			| ((p_type) << 5) \
			| (p_recipient), \
		.request = (p_request), \
		{ .value = (p_value) }, \
		.index = (p_index), \
		.length = (p_length) \
	}

/** Prepare setup packet.
 *
 * @param name Variable name with the setup packet.
 * @param p_direction Data transfer direction.
 * @param p_type Request type (standard/class/vendor)
 * @param p_recipient Recipient of the request.
 * @param p_request Request.
 * @param p_value_low wValue field of setup packet (low byte).
 * @param p_value_high wValue field of setup packet (high byte).
 * @param p_index wIndex field of setup packet.
 * @param p_length Length of extra data.
 */
#define PREPARE_SETUP_PACKET_LOHI(name, p_direction, p_type, p_recipient, \
    p_request, p_value_low, p_value_high, p_index, p_length) \
	PREPARE_SETUP_PACKET(name, p_direction, p_type, p_recipient, \
	    p_request, (p_value_low) | ((p_value_high) << 8), \
	    p_index, p_length)

/** Retrieve status of a USB device.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] recipient Recipient of the request.
 * @param[in] recipient_index Index of @p recipient.
 * @param[out] status Status (see figure 9-4 in USB 1.1 specification).
 * @return Error code.
 */
int usb_drv_req_get_status(int hc_phone, usb_address_t address,
    usb_request_recipient_t recipient, uint16_t recipient_index,
    uint16_t *status)
{
	if (status == NULL) {
		return EBADMEM;
	}

	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET(setup_packet,
	    USB_DIRECTION_IN, USB_REQUEST_TYPE_STANDARD,
	    recipient, USB_DEVREQ_GET_STATUS, 0, recipient_index, 2);

	size_t transfered;
	uint16_t tmp_status;
	int rc = usb_drv_psync_control_read(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), &tmp_status, 2, &transfered);
	if (rc != EOK) {
		return rc;
	}
	if (transfered != 2) {
		return ERANGE;
	}

	*status = tmp_status;

	return EOK;
}

/** Clear or disable USB device feature.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] recipient Recipient of the request.
 * @param[in] selector Feature selector.
 * @param[in] index Index of @p recipient.
 * @return Error code.
 */
int usb_drv_req_clear_feature(int hc_phone, usb_address_t address,
    usb_request_recipient_t recipient,
    uint16_t selector, uint16_t index)
{
	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET(setup_packet,
	    USB_DIRECTION_OUT, USB_REQUEST_TYPE_STANDARD,
	    recipient, USB_DEVREQ_CLEAR_FEATURE, selector, index, 0);

	int rc = usb_drv_psync_control_write(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), NULL, 0);

	return rc;
}

/** Set or enable USB device feature.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] recipient Recipient of the request.
 * @param[in] selector Feature selector.
 * @param[in] index Index of @p recipient.
 * @return Error code.
 */
int usb_drv_req_set_feature(int hc_phone, usb_address_t address,
    usb_request_recipient_t recipient,
    uint16_t selector, uint16_t index)
{
	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET(setup_packet,
	    USB_DIRECTION_OUT, USB_REQUEST_TYPE_STANDARD,
	    recipient, USB_DEVREQ_SET_FEATURE, selector, index, 0);

	int rc = usb_drv_psync_control_write(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), NULL, 0);

	return rc;
}

/** Change address of connected device.
 *
 * @see usb_drv_reserve_default_address
 * @see usb_drv_release_default_address
 * @see usb_drv_request_address
 * @see usb_drv_release_address
 * @see usb_drv_bind_address
 *
 * @param[in] phone Open phone to HC driver.
 * @param[in] old_address Current address.
 * @param[in] address Address to be set.
 * @return Error code.
 */
int usb_drv_req_set_address(int phone, usb_address_t old_address,
    usb_address_t new_address)
{
	PREPARE_TARGET(target, old_address);

	PREPARE_SETUP_PACKET(setup_packet, USB_DIRECTION_OUT,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_ADDRESS, new_address, 0, 0);

	int rc = usb_drv_psync_control_write(phone, target,
	    &setup_packet, sizeof(setup_packet), NULL, 0);

	return rc;
}

/** Retrieve USB descriptor of connected USB device.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
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
	if (buffer == NULL) {
		return EBADMEM;
	}
	if (size == 0) {
		return EINVAL;
	}

	// FIXME: check that size is not too big

	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET_LOHI(setup_packet, USB_DIRECTION_IN,
	    request_type, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_GET_DESCRIPTOR, descriptor_index, descriptor_type,
	    language, size);

	int rc = usb_drv_psync_control_read(hc_phone, target,
	    &setup_packet, sizeof(setup_packet),
	    buffer, size, actual_size);
	
	return rc;
}

/** Retrieve device descriptor of connected USB device.
 *
 * @param[in] phone Open phone to HC driver.
 * @param[in] address Device address.
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
	
	size_t actually_transferred = 0;
	usb_standard_device_descriptor_t descriptor_tmp;
	int rc = usb_drv_req_get_descriptor(phone, address,
	    USB_REQUEST_TYPE_STANDARD,
	    USB_DESCTYPE_DEVICE, 0,
	    0,
	    &descriptor_tmp, sizeof(descriptor_tmp),
	    &actually_transferred);

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
 * @param[in] address Device address.
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
	
	size_t actually_transferred = 0;
	usb_standard_configuration_descriptor_t descriptor_tmp;
	int rc = usb_drv_req_get_descriptor(phone, address,
	    USB_REQUEST_TYPE_STANDARD,
	    USB_DESCTYPE_CONFIGURATION, 0,
	    0,
	    &descriptor_tmp, sizeof(descriptor_tmp),
	    &actually_transferred);

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
 * even when error occurs.
 *
 * @param[in] phone Open phone to HC driver.
 * @param[in] address Device address.
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
	int rc = usb_drv_req_get_descriptor(phone, address,
	    USB_REQUEST_TYPE_STANDARD,
	    USB_DESCTYPE_CONFIGURATION, 0,
	    0,
	    buffer, buffer_size,
	    actual_buffer_size);

	return rc;
}

/** Update existing descriptor of a USB device.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] descriptor_type Descriptor type (device/configuration/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
 * @param[in] descriptor Actual descriptor data.
 * @param[in] descriptor_size Descriptor size.
 * @return Error code.
 */
int usb_drv_req_set_descriptor(int hc_phone, usb_address_t address,
    uint8_t descriptor_type, uint8_t descriptor_index,
    uint16_t language,
    void *descriptor, size_t descriptor_size)
{
	// FIXME: check that descriptor is not too big

	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET_LOHI(setup_packet, USB_DIRECTION_OUT,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_DESCRIPTOR, descriptor_index, descriptor_type,
	    language, descriptor_size);

	int rc = usb_drv_psync_control_write(hc_phone, target,
	    &setup_packet, sizeof(setup_packet),
	    descriptor, descriptor_size);

	return rc;
}

/** Determine current configuration value of USB device.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[out] configuration_value Current configuration value.
 * @return Error code.
 */
int usb_drv_req_get_configuration(int hc_phone, usb_address_t address,
    uint8_t *configuration_value)
{
	if (configuration_value == NULL) {
		return EBADMEM;
	}

	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET(setup_packet, USB_DIRECTION_IN,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_GET_CONFIGURATION, 0, 0, 1);

	uint8_t value;
	size_t transfered;
	int rc = usb_drv_psync_control_read(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), &value, 1, &transfered);

	if (rc != EOK) {
		return rc;
	}

	if (transfered != 1) {
		return ERANGE;
	}

	*configuration_value = value;

	return EOK;
}

/** Set configuration of USB device.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] configuration_value New configuration value.
 * @return Error code.
 */
int usb_drv_req_set_configuration(int hc_phone, usb_address_t address,
    uint8_t configuration_value)
{
	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET_LOHI(setup_packet, USB_DIRECTION_OUT,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_CONFIGURATION, configuration_value, 0,
	    0, 0);

	int rc = usb_drv_psync_control_write(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), NULL, 0);

	return rc;
}

/** Determine alternate setting of USB device interface.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] interface_index Interface index.
 * @param[out] alternate_setting Value of alternate setting.
 * @return Error code.
 */
int usb_drv_req_get_interface(int hc_phone, usb_address_t address,
    uint16_t interface_index, uint8_t *alternate_setting)
{
	if (alternate_setting == NULL) {
		return EBADMEM;
	}

	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET(setup_packet, USB_DIRECTION_IN,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DEVREQ_GET_INTERFACE, 0, interface_index, 1);

	uint8_t alternate;
	size_t transfered;
	int rc = usb_drv_psync_control_read(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), &alternate, 1, &transfered);

	if (rc != EOK) {
		return rc;
	}

	if (transfered != 1) {
		return ERANGE;
	}

	*alternate_setting = alternate;

	return EOK;
}

/** Select an alternate setting of USB device interface.
 *
 * @param[in] hc_phone Open phone to HC driver.
 * @param[in] address Device address.
 * @param[in] interface_index Interface index.
 * @param[in] alternate_setting Value of alternate setting.
 * @return Error code.
 */
int usb_drv_req_set_interface(int hc_phone, usb_address_t address,
    uint16_t interface_index, uint8_t alternate_setting)
{
	PREPARE_TARGET(target, address);

	PREPARE_SETUP_PACKET_LOHI(setup_packet, USB_DIRECTION_OUT,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DEVREQ_SET_INTERFACE, alternate_setting, 0,
	    0, 0);

	int rc = usb_drv_psync_control_write(hc_phone, target,
	    &setup_packet, sizeof(setup_packet), NULL, 0);

	return rc;
}

/**
 * @}
 */
