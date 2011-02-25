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
#include <assert.h>

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

/** Retrieve USB descriptor, allocate space for it.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
 * @param[out] buffer_ptr Where to store pointer to allocated buffer.
 * @param[out] buffer_size Where to store the size of the descriptor.
 * @return
 */
int usb_request_get_descriptor_alloc(usb_endpoint_pipe_t * pipe,
    usb_request_type_t request_type,
    uint8_t descriptor_type, uint8_t descriptor_index,
    uint16_t language,
    void **buffer_ptr, size_t *buffer_size)
{
	if (buffer_ptr == NULL) {
		return EBADMEM;
	}

	int rc;

	/*
	 * Get only first byte to retrieve descriptor length.
	 */
	uint8_t tmp_buffer[1];
	size_t bytes_transfered;
	rc = usb_request_get_descriptor(pipe, request_type,
	    descriptor_type, descriptor_index, language,
	    &tmp_buffer, 1, &bytes_transfered);
	if (rc != EOK) {
		return rc;
	}
	if (bytes_transfered != 1) {
		/* FIXME: some better error code? */
		return ESTALL;
	}

	size_t size = tmp_buffer[0];
	if (size == 0) {
		/* FIXME: some better error code? */
		return ESTALL;
	}

	/*
	 * Allocate buffer and get the descriptor again.
	 */
	void *buffer = malloc(size);
	if (buffer == NULL) {
		return ENOMEM;
	}

	rc = usb_request_get_descriptor(pipe, request_type,
	    descriptor_type, descriptor_index, language,
	    buffer, size, &bytes_transfered);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}
	if (bytes_transfered != size) {
		free(buffer);
		/* FIXME: some better error code? */
		return ESTALL;
	}

	*buffer_ptr = buffer;
	if (buffer_size != NULL) {
		*buffer_size = size;
	}

	return EOK;
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

/** Get list of supported languages by USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[out] languages_ptr Where to store pointer to allocated array of
 *	supported languages.
 * @param[out] languages_count Number of supported languages.
 * @return Error code.
 */
int usb_request_get_supported_languages(usb_endpoint_pipe_t *pipe,
    l18_win_locales_t **languages_ptr, size_t *languages_count)
{
	int rc;

	if (languages_ptr == NULL) {
		return EBADMEM;
	}
	if (languages_count == NULL) {
		return EBADMEM;
	}

	uint8_t *string_descriptor = NULL;
	size_t string_descriptor_size = 0;
	rc = usb_request_get_descriptor_alloc(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_STRING, 0, 0,
	    (void **) &string_descriptor, &string_descriptor_size);
	if (rc != EOK) {
		return rc;
	}
	if (string_descriptor_size <= 2) {
		free(string_descriptor);
		return EEMPTY;
	}
	/* Substract first 2 bytes (length and descriptor type). */
	string_descriptor_size -= 2;

	/* Odd number of bytes - descriptor is broken? */
	if ((string_descriptor_size % 2) != 0) {
		/* FIXME: shall we return with error or silently ignore? */
		free(string_descriptor);
		return ESTALL;
	}

	size_t langs_count = string_descriptor_size / 2;
	l18_win_locales_t *langs
	    = malloc(sizeof(l18_win_locales_t) * langs_count);
	if (langs == NULL) {
		free(string_descriptor);
		return ENOMEM;
	}

	size_t i;
	for (i = 0; i < langs_count; i++) {
		/* Language code from the descriptor is in USB endianess. */
		/* FIXME: is this really correct? */
		uint16_t lang_code = (string_descriptor[2 + 2 * i + 1] << 8)
		    + string_descriptor[2 + 2 * i];
		langs[i] = uint16_usb2host(lang_code);
	}

	free(string_descriptor);

	*languages_ptr = langs;
	*languages_count =langs_count;

	return EOK;
}

/** Get string (descriptor) from USB device.
 *
 * The string is returned in native encoding of the operating system.
 * For HelenOS, that is UTF-8.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] index String index (in native endianess),
 *	first index has number 1 (index from descriptors can be used directly).
 * @param[in] lang String language (in native endianess).
 * @param[out] string_ptr Where to store allocated string in native encoding.
 * @return Error code.
 */
int usb_request_get_string(usb_endpoint_pipe_t *pipe,
    size_t index, l18_win_locales_t lang, char **string_ptr)
{
	if (string_ptr == NULL) {
		return EBADMEM;
	}
	/*
	 * Index is actually one byte value and zero index is used
	 * to retrieve list of supported languages.
	 */
	if ((index < 1) || (index > 0xFF)) {
		return ERANGE;
	}
	/* Language is actually two byte value. */
	if (lang > 0xFFFF) {
		return ERANGE;
	}

	int rc;

	/* Prepare dynamically allocated variables. */
	uint8_t *string = NULL;
	wchar_t *string_chars = NULL;

	/* Get the actual descriptor. */
	size_t string_size;
	rc = usb_request_get_descriptor_alloc(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_DESCTYPE_STRING,
	    index, uint16_host2usb(lang),
	    (void **) &string, &string_size);
	if (rc != EOK) {
		goto leave;
	}

	if (string_size <= 2) {
		rc =  EEMPTY;
		goto leave;
	}
	/* Substract first 2 bytes (length and descriptor type). */
	string_size -= 2;

	/* Odd number of bytes - descriptor is broken? */
	if ((string_size % 2) != 0) {
		/* FIXME: shall we return with error or silently ignore? */
		rc = ESTALL;
		goto leave;
	}

	size_t string_char_count = string_size / 2;
	string_chars = malloc(sizeof(wchar_t) * (string_char_count + 1));
	if (string_chars == NULL) {
		rc = ENOMEM;
		goto leave;
	}

	/*
	 * Build a wide string.
	 * And do not forget to set NULL terminator (string descriptors
	 * do not have them).
	 */
	size_t i;
	for (i = 0; i < string_char_count; i++) {
		uint16_t uni_char = (string[2 + 2 * i + 1] << 8)
		    + string[2 + 2 * i];
		string_chars[i] = uni_char;
	}
	string_chars[string_char_count] = 0;


	/* Convert to normal string. */
	char *str = wstr_to_astr(string_chars);
	if (str == NULL) {
		rc = ENOMEM;
		goto leave;
	}

	*string_ptr = str;
	rc = EOK;

leave:
	if (string != NULL) {
		free(string);
	}
	if (string_chars != NULL) {
		free(string_chars);
	}

	return rc;
}

/**
 * @}
 */
