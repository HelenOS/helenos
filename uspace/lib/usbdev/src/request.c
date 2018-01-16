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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Standard USB requests (implementation).
 */
#include <usb/dev/request.h>
#include <usb/request.h>
#include <usb/usb.h>

#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>

#define MAX_DATA_LENGTH ((size_t)(0xFFFF))

static_assert(sizeof(usb_device_request_setup_packet_t) == 8);

/** Generic wrapper for SET requests using standard control request format.
 *
 * @see usb_pipe_control_write
 *
 * @param pipe         Pipe used for the communication.
 * @param request_type Request type (standard/class/vendor).
 * @param recipient    Request recipient (e.g. device or endpoint).
 * @param request      Actual request (e.g. GET_DESCRIPTOR).
 * @param value        Value of @c wValue field of setup packet
 *                     (must be in USB endianness).
 * @param index        Value of @c wIndex field of setup packet
 *                     (must be in USB endianness).
 * @param data         Data to be sent during DATA stage
 *                     (expected to be in USB endianness).
 * @param data_size     Size of the @p data buffer (in native endianness).
 *
 * @return Error code.
 * @retval EBADMEM @p pipe is NULL.
 * @retval EBADMEM @p data is NULL and @p data_size is not zero.
 * @retval ERANGE Data buffer too large.
 *
 */
errno_t usb_control_request_set(usb_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint8_t request, uint16_t value, uint16_t index,
    const void *data, size_t data_size)
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

	const usb_device_request_setup_packet_t setup_packet = {
		.request_type = (request_type << 5) | recipient,
		.request = request,
		.value = value,
		.index = index,
		.length = (uint16_t) data_size,
	};

	return usb_pipe_control_write(pipe,
	    &setup_packet, sizeof(setup_packet), data, data_size);
}

/** Generic wrapper for GET requests using standard control request format.
 *
 * @see usb_pipe_control_read
 *
 * @param pipe             Pipe used for the communication.
 * @param request_type     Request type (standard/class/vendor).
 * @param recipient        Request recipient (e.g. device or endpoint).
 * @param request          Actual request (e.g. GET_DESCRIPTOR).
 * @param value            Value of @c wValue field of setup packet
 *                         (must be in USB endianness).
 * @param index            Value of @c wIndex field of setup packet
 *                         (must be in USB endianness).
 * @param data             Buffer where to store data accepted during
 *                         the DATA stage (they will come in USB endianness).
 * @param data_size        Size of the @p data buffer
 *                         (in native endianness).
 * @param actual_data_size Actual size of transfered data
 *                         (in native endianness).
 *
 * @return Error code.
 * @retval EBADMEM @p pipe is NULL.
 * @retval EBADMEM @p data is NULL and @p data_size is not zero.
 * @retval ERANGE Data buffer too large.
 *
 */
errno_t usb_control_request_get(usb_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint8_t request, uint16_t value, uint16_t index,
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

	const usb_device_request_setup_packet_t setup_packet = {
		.request_type = SETUP_REQUEST_TYPE_DEVICE_TO_HOST
		    | (request_type << 5) | recipient,
		.request = request,
		.value = uint16_host2usb(value),
		.index = uint16_host2usb(index),
		.length = uint16_host2usb(data_size),
	};

	return usb_pipe_control_read(pipe, &setup_packet, sizeof(setup_packet),
	    data, data_size, actual_data_size);
}

/** Retrieve status of a USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] index Recipient index (in native endianness).
 * @param[in] recipient Recipient of the GET_STATUS request.
 * @param[out] status Recipient status (in native endianness).
 * @return Error code.
 */
errno_t usb_request_get_status(usb_pipe_t *pipe,
    usb_request_recipient_t recipient, uint16_t index,
    uint16_t *status)
{
	if ((recipient == USB_REQUEST_RECIPIENT_DEVICE) && (index != 0)) {
		return EINVAL;
	}

	if (status == NULL) {
		return EBADMEM;
	}

	uint16_t status_usb_endianess;
	size_t data_transfered_size;
	errno_t rc = usb_control_request_get(pipe, USB_REQUEST_TYPE_STANDARD,
	    recipient, USB_DEVREQ_GET_STATUS, 0, uint16_host2usb(index),
	    &status_usb_endianess, 2, &data_transfered_size);
	if (rc != EOK) {
		return rc;
	}
	if (data_transfered_size != 2) {
		return ELIMIT;
	}

	*status = uint16_usb2host(status_usb_endianess);

	return EOK;
}

/** Clear or disable specific device feature.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] recipient Recipient of the CLEAR_FEATURE request.
 * @param[in] feature_selector Feature selector (in native endianness).
 * @param[in] index Recipient index (in native endianness).
 * @return Error code.
 */
errno_t usb_request_clear_feature(usb_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint16_t feature_selector, uint16_t index)
{
	if (request_type == USB_REQUEST_TYPE_STANDARD) {
		if ((recipient == USB_REQUEST_RECIPIENT_DEVICE) && (index != 0)) {
			return EINVAL;
		}
	}

	return usb_control_request_set(pipe,
	    request_type, recipient, USB_DEVREQ_CLEAR_FEATURE,
	    uint16_host2usb(feature_selector), uint16_host2usb(index), NULL, 0);
}

/** Set or enable specific device feature.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] recipient Recipient of the SET_FEATURE request.
 * @param[in] feature_selector Feature selector (in native endianness).
 * @param[in] index Recipient index (in native endianness).
 * @return Error code.
 */
errno_t usb_request_set_feature(usb_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint16_t feature_selector, uint16_t index)
{
	if (request_type == USB_REQUEST_TYPE_STANDARD) {
		if ((recipient == USB_REQUEST_RECIPIENT_DEVICE) && (index != 0)) {
			return EINVAL;
		}
	}

	return usb_control_request_set(pipe,
	    request_type, recipient, USB_DEVREQ_SET_FEATURE,
	    uint16_host2usb(feature_selector), uint16_host2usb(index), NULL, 0);
}

/** Retrieve USB descriptor of a USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] recipient Request recipient (device/interface/endpoint).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
 * @param[out] buffer Buffer where to store the retrieved descriptor.
 * @param[in] size Size of the @p buffer.
 * @param[out] actual_size Number of bytes actually transferred.
 * @return Error code.
 */
errno_t usb_request_get_descriptor(usb_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
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

	/*
	 * The wValue field specifies the descriptor type in the high byte
	 * and the descriptor index in the low byte. USB 1.1 spec p. 189
	 */
	const uint16_t wValue = descriptor_index | (descriptor_type << 8);

	return usb_control_request_get(pipe,
	    request_type, recipient,
	    USB_DEVREQ_GET_DESCRIPTOR,
	    wValue, language,
	    buffer, size, actual_size);
}

/** Retrieve USB descriptor, allocate space for it.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] recipient Request recipient (device/interface/endpoint).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index.
 * @param[out] buffer_ptr Where to store pointer to allocated buffer.
 * @param[out] buffer_size Where to store the size of the descriptor.
 * @return
 */
errno_t usb_request_get_descriptor_alloc(usb_pipe_t * pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint8_t descriptor_type, uint8_t descriptor_index,
    uint16_t language,
    void **buffer_ptr, size_t *buffer_size)
{
	if (buffer_ptr == NULL) {
		return EBADMEM;
	}

	errno_t rc;

	/*
	 * Get only first byte to retrieve descriptor length.
	 */
	uint8_t tmp_buffer;
	size_t bytes_transfered;
	rc = usb_request_get_descriptor(pipe, request_type, recipient,
	    descriptor_type, descriptor_index, language,
	    &tmp_buffer, sizeof(tmp_buffer), &bytes_transfered);
	if (rc != EOK) {
		return rc;
	}
	if (bytes_transfered != 1) {
		return ELIMIT;
	}

	const size_t size = tmp_buffer;
	if (size == 0) {
		return ELIMIT;
	}

	/*
	 * Allocate buffer and get the descriptor again.
	 */
	void *buffer = malloc(size);
	if (buffer == NULL) {
		return ENOMEM;
	}

	rc = usb_request_get_descriptor(pipe, request_type, recipient,
	    descriptor_type, descriptor_index, language,
	    buffer, size, &bytes_transfered);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}
	if (bytes_transfered != size) {
		free(buffer);
		return ELIMIT;
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
errno_t usb_request_get_device_descriptor(usb_pipe_t *pipe,
    usb_standard_device_descriptor_t *descriptor)
{
	if (descriptor == NULL) {
		return EBADMEM;
	}

	size_t actually_transferred = 0;
	usb_standard_device_descriptor_t descriptor_tmp;
	errno_t rc = usb_request_get_descriptor(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_DEVICE, 0, 0, &descriptor_tmp, sizeof(descriptor_tmp),
	    &actually_transferred);

	if (rc != EOK) {
		return rc;
	}

	/* Verify that all data has been transferred. */
	if (actually_transferred < sizeof(descriptor_tmp)) {
		return ELIMIT;
	}

	/* Everything is okay, copy the descriptor. */
	memcpy(descriptor, &descriptor_tmp, sizeof(descriptor_tmp));

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
errno_t usb_request_get_bare_configuration_descriptor(usb_pipe_t *pipe,
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
	const errno_t rc = usb_request_get_descriptor(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_CONFIGURATION, index, 0,
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
	memcpy(descriptor, &descriptor_tmp, sizeof(descriptor_tmp));
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
errno_t usb_request_get_full_configuration_descriptor(usb_pipe_t *pipe,
    int index, void *descriptor, size_t descriptor_size, size_t *actual_size)
{
	if ((index < 0) || (index > 0xFF)) {
		return ERANGE;
	}

	return usb_request_get_descriptor(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_CONFIGURATION, index, 0,
	    descriptor, descriptor_size, actual_size);
}

/** Retrieve full configuration descriptor, allocate space for it.
 *
 * The function takes care that full configuration descriptor is returned
 * (i.e. the function will fail when less data then descriptor.totalLength
 * is returned).
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] index Configuration index.
 * @param[out] descriptor_ptr Where to store pointer to allocated buffer.
 * @param[out] descriptor_size Where to store the size of the descriptor.
 * @return Error code.
 */
errno_t usb_request_get_full_configuration_descriptor_alloc(
    usb_pipe_t *pipe, int index,
    void **descriptor_ptr, size_t *descriptor_size)
{
	errno_t rc;

	if (descriptor_ptr == NULL) {
		return EBADMEM;
	}

	usb_standard_configuration_descriptor_t bare_config;
	rc = usb_request_get_bare_configuration_descriptor(pipe, index,
	    &bare_config);
	if (rc != EOK) {
		return rc;
	}
	if (bare_config.descriptor_type != USB_DESCTYPE_CONFIGURATION) {
		return ENOENT;
	}

	const size_t total_length = uint16_usb2host(bare_config.total_length);
	if (total_length < sizeof(bare_config)) {
		return ELIMIT;
	}

	void *buffer = malloc(total_length);
	if (buffer == NULL) {
		return ENOMEM;
	}

	size_t transferred = 0;
	rc = usb_request_get_full_configuration_descriptor(pipe, index,
	    buffer, total_length, &transferred);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}

	if (transferred != total_length) {
		free(buffer);
		return ELIMIT;
	}

	/* Everything looks okay, copy the pointers. */

	*descriptor_ptr = buffer;

	if (descriptor_size != NULL) {
		*descriptor_size = total_length;
	}

	return EOK;
}

/** Update existing or add new USB descriptor to a USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] request_type Request type (standard/class/vendor).
 * @param[in] recipient Request recipient (device/interface/endpoint).
 * @param[in] descriptor_type Descriptor type (device/configuration/HID/...).
 * @param[in] descriptor_index Descriptor index.
 * @param[in] language Language index (in native endianness).
 * @param[in] buffer Buffer with the new descriptor (in USB endianness).
 * @param[in] size Size of the @p buffer in bytes (in native endianness).
 * @return Error code.
 */
errno_t usb_request_set_descriptor(usb_pipe_t *pipe,
    usb_request_type_t request_type, usb_request_recipient_t recipient,
    uint8_t descriptor_type, uint8_t descriptor_index,
    uint16_t language, const void *buffer, size_t size)
{
	if (buffer == NULL) {
		return EBADMEM;
	}
	if (size == 0) {
		return EINVAL;
	}

	/* FIXME: proper endianness. */
	uint16_t wValue = descriptor_index | (descriptor_type << 8);

	return usb_control_request_set(pipe,
	    request_type, recipient, USB_DEVREQ_SET_DESCRIPTOR,
	    wValue, language, buffer, size);
}

/** Get current configuration value of USB device.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[out] configuration_value Current configuration value.
 * @return Error code.
 */
errno_t usb_request_get_configuration(usb_pipe_t *pipe,
    uint8_t *configuration_value)
{
	uint8_t value;
	size_t actual_size;

	const errno_t rc = usb_control_request_get(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_GET_CONFIGURATION, 0, 0, &value, 1, &actual_size);

	if (rc != EOK) {
		return rc;
	}
	if (actual_size != 1) {
		return ELIMIT;
	}

	if (configuration_value != NULL) {
		*configuration_value = value;
	}

	return EOK;
}

/** Set configuration of USB device.
 *
 * @param pipe Control endpoint pipe (session must be already started).
 * @param configuration_value New configuration value.
 * @return Error code.
 */
errno_t usb_request_set_configuration(usb_pipe_t *pipe,
    uint8_t configuration_value)
{
	const uint16_t config_value
	    = uint16_host2usb((uint16_t) configuration_value);

	return usb_control_request_set(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DEVREQ_SET_CONFIGURATION, config_value, 0,
	    NULL, 0);
}

/** Get selected alternate setting for USB interface.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] interface_index Interface index.
 * @param[out] alternate_setting Alternate setting for the interface.
 * @return Error code.
 */
errno_t usb_request_get_interface(usb_pipe_t *pipe,
    uint8_t interface_index, uint8_t *alternate_setting)
{
	uint8_t value;
	size_t actual_size;

	const errno_t rc = usb_control_request_get(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DEVREQ_GET_INTERFACE,
	    0, uint16_host2usb((uint16_t) interface_index),
	    &value, sizeof(value), &actual_size);

	if (rc != EOK) {
		return rc;
	}
	if (actual_size != 1) {
		return ELIMIT;
	}

	if (alternate_setting != NULL) {
		*alternate_setting = value;
	}

	return EOK;
}

/** Select alternate setting for USB interface.
 *
 * @param[in] pipe Control endpoint pipe (session must be already started).
 * @param[in] interface_index Interface index.
 * @param[in] alternate_setting Alternate setting to select.
 * @return Error code.
 */
errno_t usb_request_set_interface(usb_pipe_t *pipe,
    uint8_t interface_index, uint8_t alternate_setting)
{
	return usb_control_request_set(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DEVREQ_SET_INTERFACE,
	    uint16_host2usb((uint16_t) alternate_setting),
	    uint16_host2usb((uint16_t) interface_index),
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
errno_t usb_request_get_supported_languages(usb_pipe_t *pipe,
    l18_win_locales_t **languages_ptr, size_t *languages_count)
{
	if (languages_ptr == NULL || languages_count == NULL) {
		return EBADMEM;
	}

	uint8_t *string_descriptor = NULL;
	size_t string_descriptor_size = 0;
	const errno_t rc = usb_request_get_descriptor_alloc(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_STRING, 0, 0,
	    (void **) &string_descriptor, &string_descriptor_size);
	if (rc != EOK) {
		return rc;
	}
	if (string_descriptor_size <= 2) {
		free(string_descriptor);
		return EEMPTY;
	}
	/* Subtract first 2 bytes (length and descriptor type). */
	string_descriptor_size -= 2;

	/* Odd number of bytes - descriptor is broken? */
	if ((string_descriptor_size % 2) != 0) {
		/* FIXME: shall we return with error or silently ignore? */
		free(string_descriptor);
		return ESTALL;
	}

	const size_t langs_count = string_descriptor_size / 2;
	l18_win_locales_t *langs =
	    calloc(langs_count, sizeof(l18_win_locales_t));
	if (langs == NULL) {
		free(string_descriptor);
		return ENOMEM;
	}

	for (size_t i = 0; i < langs_count; i++) {
		/* Language code from the descriptor is in USB endianness. */
		/* FIXME: is this really correct? */
		const uint16_t lang_code =
		    (string_descriptor[2 + 2 * i + 1] << 8)
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
 * @param[in] index String index (in native endianness),
 *	first index has number 1 (index from descriptors can be used directly).
 * @param[in] lang String language (in native endianness).
 * @param[out] string_ptr Where to store allocated string in native encoding.
 * @return Error code.
 */
errno_t usb_request_get_string(usb_pipe_t *pipe,
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
	if (lang > L18N_WIN_LOCALE_MAX) {
		return ERANGE;
	}

	errno_t rc;

	/* Prepare dynamically allocated variables. */
	uint8_t *string = NULL;
	wchar_t *string_chars = NULL;

	/* Get the actual descriptor. */
	size_t string_size;
	rc = usb_request_get_descriptor_alloc(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_DEVICE,
	    USB_DESCTYPE_STRING, index, uint16_host2usb(lang),
	    (void **) &string, &string_size);
	if (rc != EOK) {
		goto leave;
	}

	if (string_size <= 2) {
		rc =  EEMPTY;
		goto leave;
	}
	/* Subtract first 2 bytes (length and descriptor type). */
	string_size -= 2;

	/* Odd number of bytes - descriptor is broken? */
	if ((string_size % 2) != 0) {
		/* FIXME: shall we return with error or silently ignore? */
		rc = ESTALL;
		goto leave;
	}

	const size_t string_char_count = string_size / 2;
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
	for (size_t i = 0; i < string_char_count; i++) {
		const uint16_t uni_char = (string[2 + 2 * i + 1] << 8)
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
	free(string);
	free(string_chars);

	return rc;
}

/** Clear halt bit of an endpoint pipe (after pipe stall).
 *
 * @param pipe Control pipe.
 * @param ep_index Endpoint index (in native endianness).
 * @return Error code.
 */
errno_t usb_request_clear_endpoint_halt(usb_pipe_t *pipe, uint16_t ep_index)
{
	return usb_request_clear_feature(pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_ENDPOINT,
	    uint16_host2usb(USB_FEATURE_ENDPOINT_HALT),
	    uint16_host2usb(ep_index));
}

/** Clear halt bit of an endpoint pipe (after pipe stall).
 *
 * @param ctrl_pipe Control pipe.
 * @param target_pipe Which pipe is halted and shall be cleared.
 * @return Error code.
 */
errno_t usb_pipe_clear_halt(usb_pipe_t *ctrl_pipe, usb_pipe_t *target_pipe)
{
	if ((ctrl_pipe == NULL) || (target_pipe == NULL)) {
		return EINVAL;
	}
	return usb_request_clear_endpoint_halt(ctrl_pipe,
	    target_pipe->endpoint_no);
}

/** Get endpoint status.
 *
 * @param[in] ctrl_pipe Control pipe.
 * @param[in] pipe Of which pipe the status shall be received.
 * @param[out] status Where to store pipe status (in native endianness).
 * @return Error code.
 */
errno_t usb_request_get_endpoint_status(usb_pipe_t *ctrl_pipe, usb_pipe_t *pipe,
    uint16_t *status)
{
	uint16_t status_tmp;
	uint16_t pipe_index = (uint16_t) pipe->endpoint_no;
	errno_t rc = usb_request_get_status(ctrl_pipe,
	    USB_REQUEST_RECIPIENT_ENDPOINT, uint16_host2usb(pipe_index),
	    &status_tmp);
	if (rc != EOK) {
		return rc;
	}

	if (status != NULL) {
		*status = uint16_usb2host(status_tmp);
	}

	return EOK;
}

/**
 * @}
 */
