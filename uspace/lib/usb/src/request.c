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
/** @file
 * Standard USB requests (implementation).
 */
#include <usb/request.h>
#include <errno.h>

#define MAX_DATA_LENGTH ((size_t)(0xFFFF))

/** Generic wrapper for SET requests using standard control request format.
 *
 * @see usb_endpoint_pipe_control_write
 *
 * @param pipe Pipe used for the communication.
 * @param request_type Request type (standard/class/vendor).
 * @param recipient Request recipient (e.g. device or endpoint).
 * @param request Actual request (e.g. GET_DESCRIPTOR).
 * @param value Value of @c wValue field of setup packet
 * 	(must be in USB endianness).
 * @param index Value of @c wIndex field of setup packet
 * 	(must be in USB endianness).
 * @param data Data to be sent during DATA stage
 * 	(expected to be in USB endianness).
 * @param data_size Size of the @p data buffer (in native endianness).
 * @return Error code.
 * @retval EBADMEM @p pipe is NULL.
 * @retval EBADMEM @p data is NULL and @p data_size is not zero.
 * @retval ERANGE Data buffer too large.
 */
int usb_control_request_set(usb_endpoint_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint8_t request,
    uint16_t value, uint16_t index,
    void *data, size_t data_size)
{
	if (pipe == NULL) {
		return EBADMEM;
	}

	if (data_size > MAX_DATA_LENGTH) {
		return ERANGE;
	}

	if ((data_size > 0) && (data == NULL)) {
		return EBADMEM;
	}

	/*
	 * TODO: check that @p request_type and @p recipient are
	 * within ranges.
	 */

	usb_device_request_setup_packet_t setup_packet;
	setup_packet.request_type = (request_type << 5) | recipient;
	setup_packet.request = request;
	setup_packet.value = value;
	setup_packet.index = index;
	setup_packet.length = (uint16_t) data_size;

	int rc = usb_endpoint_pipe_control_write(pipe,
	    &setup_packet, sizeof(setup_packet),
	    data, data_size);

	return rc;
}

 /** Generic wrapper for GET requests using standard control request format.
  *
  * @see usb_endpoint_pipe_control_read
  *
  * @param pipe Pipe used for the communication.
  * @param request_type Request type (standard/class/vendor).
  * @param recipient Request recipient (e.g. device or endpoint).
  * @param request Actual request (e.g. GET_DESCRIPTOR).
  * @param value Value of @c wValue field of setup packet
  * 	(must be in USB endianness).
  * @param index Value of @c wIndex field of setup packet
  *	(must be in USB endianness).
  * @param data Buffer where to store data accepted during the DATA stage.
  *	(they will come in USB endianess).
  * @param data_size Size of the @p data buffer
  * 	(in native endianness).
  * @param actual_data_size Actual size of transfered data
  * 	(in native endianness).
  * @return Error code.
  * @retval EBADMEM @p pipe is NULL.
  * @retval EBADMEM @p data is NULL and @p data_size is not zero.
  * @retval ERANGE Data buffer too large.
  */
int usb_control_request_get(usb_endpoint_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint8_t request,
    uint16_t value, uint16_t index,
    void *data, size_t data_size, size_t *actual_data_size)
{
	if (pipe == NULL) {
		return EBADMEM;
	}

	if (data_size > MAX_DATA_LENGTH) {
		return ERANGE;
	}

	if ((data_size > 0) && (data == NULL)) {
		return EBADMEM;
	}

	/*
	 * TODO: check that @p request_type and @p recipient are
	 * within ranges.
	 */

	usb_device_request_setup_packet_t setup_packet;
	setup_packet.request_type = 128 | (request_type << 5) | recipient;
	setup_packet.request = request;
	setup_packet.value = value;
	setup_packet.index = index;
	setup_packet.length = (uint16_t) data_size;

	int rc = usb_endpoint_pipe_control_read(pipe,
	    &setup_packet, sizeof(setup_packet),
	    data, data_size, actual_data_size);

	return rc;
}

/** Change address of connected device.
 * This function automatically updates the backing connection to point to
 * the new address.
 *
 * @see usb_drv_reserve_default_address
 * @see usb_drv_release_default_address
 * @see usb_drv_request_address
 * @see usb_drv_release_address
 * @see usb_drv_bind_address
 *
 * @param pipe Control endpoint pipe (session must be already started).
 * @param new_address New USB address to be set (in native endianness).
 * @return Error code.
 */
int usb_request_set_address(usb_endpoint_pipe_t *pipe,
    usb_address_t new_address)
{
	if ((new_address < 0) || (new_address >= USB11_ADDRESS_MAX)) {
		return EINVAL;
	}

	uint16_t addr = uint16_host2usb((uint16_t)new_address);

	int rc = usb_control_request_set(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_ADDRESS,
	    addr, 0,
	    NULL, 0);

	if (rc != EOK) {
		return rc;
	}

	assert(pipe->wire != NULL);
	/* TODO: prevent other from accessing wire now. */
	pipe->wire->address = new_address;

	return EOK;
}

/** Retrieve USB descriptor of a USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
 * @param[out] buffer Buffer where to store the retrieved descriptor.
 * @param[in] size Size of the @p buffer.
 * @param[out] actual_size Number of bytes actually transferred.
 * @return Error code.
 */
int usb_request_get_descriptor(usb_endpoint_pipe_t *pipe,
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

	uint16_t wValue = descriptor_index | (descriptor_type << 8);

	return usb_control_request_get(pipe,
	    request_type, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_GET_DESCRIPTOR,
	    wValue, language,
	    buffer, size, actual_size);
}

/** Retrieve standard device descriptor of a USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[out] descriptor Storage for the device descriptor.
 * @return Error code.
 */
int usb_request_get_device_descriptor(usb_endpoint_pipe_t *pipe,
    usb_standard_device_descriptor_t *descriptor)
{
	if (descriptor == NULL) {
		return EBADMEM;
	}

	size_t actually_transferred = 0;
	usb_standard_device_descriptor_t descriptor_tmp;
	int rc = usb_request_get_descriptor(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_DEVICE,
	    0, 0,
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

/** Retrieve configuration descriptor of a USB device.
 *
 * The function does not retrieve additional data binded with configuration
 * descriptor (such as its interface and endpoint descriptors) - use
 * usb_request_get_full_configuration_descriptor() instead.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] index Descriptor index.
 * @param[out] descriptor Storage for the device descriptor.
 * @return Error code.
 */
int usb_request_get_bare_configuration_descriptor(usb_endpoint_pipe_t *pipe,
    int index, usb_standard_configuration_descriptor_t *descriptor)
{
	if (descriptor == NULL) {
		return EBADMEM;
	}

	if ((index < 0) || (index > 0xFF)) {
		return ERANGE;
	}

	size_t actually_transferred = 0;
	usb_standard_configuration_descriptor_t descriptor_tmp;
	int rc = usb_request_get_descriptor(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_CONFIGURATION,
	    index, 0,
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

/** Retrieve full configuration descriptor of a USB device.
 *
 * @warning The @p buffer might be touched (i.e. its contents changed)
 * even when error occurs.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] index Descriptor index.
 * @param[out] descriptor Storage for the device descriptor.
 * @param[in] descriptor_size Size of @p descriptor buffer.
 * @param[out] actual_size Number of bytes actually transferred.
 * @return Error code.
 */
int usb_request_get_full_configuration_descriptor(usb_endpoint_pipe_t *pipe,
    int index, void *descriptor, size_t descriptor_size, size_t *actual_size)
{
	if ((index < 0) || (index > 0xFF)) {
		return ERANGE;
	}

	return usb_request_get_descriptor(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_CONFIGURATION,
	    index, 0,
	    descriptor, descriptor_size, actual_size);
}

/** Set configuration of USB device.
 *
 * @param pipe Control endpoint pipe (session must be already started).
 * @param configuration_value New configuration value.
 * @return Error code.
 */
int usb_request_set_configuration(usb_endpoint_pipe_t *pipe,
    uint8_t configuration_value)
{
	uint16_t config_value
	    = uint16_host2usb((uint16_t) configuration_value);

	return usb_control_request_set(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_CONFIGURATION, config_value, 0,
	    NULL, 0);
}

/**
 * @}
 */
