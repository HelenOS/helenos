/*
 * Copyright (c) 2014 Jan Kolarik
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

/** @file wmi.c
 *
 * Implementation of Atheros WMI communication.
 *
 */

#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <mem.h>
#include <byteorder.h>
#include "wmi.h"

/** WMI registry read.
 *
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be read.
 * @param res        Stored result.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_reg_read(htc_device_t *htc_device, uint32_t reg_offset, uint32_t *res)
{
	uint32_t cmd_value = host2uint32_t_be(reg_offset);

	void *resp_buffer =
	    malloc(htc_device->ath_device->ctrl_response_length);

	errno_t rc = wmi_send_command(htc_device, WMI_REG_READ,
	    (uint8_t *) &cmd_value, sizeof(cmd_value), resp_buffer);

	if (rc != EOK) {
		usb_log_error("Failed to read registry value.\n");
		return rc;
	}

	uint32_t *resp_value = (uint32_t *) ((void *) resp_buffer +
	    sizeof(htc_frame_header_t) + sizeof(wmi_command_header_t));

	*res = uint32_t_be2host(*resp_value);

	return rc;
}

/** WMI registry write.
 *
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be written.
 * @param val        Value to be written
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_reg_write(htc_device_t *htc_device, uint32_t reg_offset, uint32_t val)
{
	uint32_t cmd_buffer[] = {
		host2uint32_t_be(reg_offset),
		host2uint32_t_be(val)
	};

	void *resp_buffer =
	    malloc(htc_device->ath_device->ctrl_response_length);

	errno_t rc = wmi_send_command(htc_device, WMI_REG_WRITE,
	    (uint8_t *) &cmd_buffer, sizeof(cmd_buffer), resp_buffer);

	free(resp_buffer);

	if (rc != EOK) {
		usb_log_error("Failed to write registry value.\n");
		return rc;
	}

	return rc;
}

/** WMI registry set or clear specified bits.
 *
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be written.
 * @param set_bit    Bit to be set.
 * @param clear_bit  Bit to be cleared.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_reg_set_clear_bit(htc_device_t *htc_device, uint32_t reg_offset,
    uint32_t set_bit, uint32_t clear_bit)
{
	uint32_t value;

	errno_t rc = wmi_reg_read(htc_device, reg_offset, &value);
	if (rc != EOK) {
		usb_log_error("Failed to read registry value in RMW "
		    "function.\n");
		return rc;
	}

	value &= ~clear_bit;
	value |= set_bit;

	rc = wmi_reg_write(htc_device, reg_offset, value);
	if (rc != EOK) {
		usb_log_error("Failed to write registry value in RMW "
		    "function.\n");
		return rc;
	}

	return rc;
}

/** WMI registry set specified bit.
 *
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be written.
 * @param set_bit    Bit to be set.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_reg_set_bit(htc_device_t *htc_device, uint32_t reg_offset,
    uint32_t set_bit)
{
	return wmi_reg_set_clear_bit(htc_device, reg_offset, set_bit, 0);
}

/** WMI registry clear specified bit.
 *
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be written.
 * @param clear_bit  Bit to be cleared.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_reg_clear_bit(htc_device_t *htc_device, uint32_t reg_offset,
    uint32_t clear_bit)
{
	return wmi_reg_set_clear_bit(htc_device, reg_offset, 0, clear_bit);
}

/** WMI multi registry write.
 *
 * @param htc_device HTC device structure.
 * @param reg_buffer Array of registry values to be written.
 * @param elements   Number of elements in array.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_reg_buffer_write(htc_device_t *htc_device, wmi_reg_t *reg_buffer,
    size_t elements)
{
	size_t buffer_size = sizeof(wmi_reg_t) * elements;
	void *buffer = malloc(buffer_size);
	void *resp_buffer =
	    malloc(htc_device->ath_device->ctrl_response_length);

	/* Convert values to correct endianness. */
	for (size_t i = 0; i < elements; i++) {
		wmi_reg_t *buffer_element = &reg_buffer[i];
		wmi_reg_t *buffer_it = (wmi_reg_t *)
		    ((void *) buffer + i * sizeof(wmi_reg_t));
		buffer_it->offset =
		    host2uint32_t_be(buffer_element->offset);
		buffer_it->value =
		    host2uint32_t_be(buffer_element->value);
	}

	errno_t rc = wmi_send_command(htc_device, WMI_REG_WRITE,
	    (uint8_t *) buffer, buffer_size, resp_buffer);

	free(buffer);
	free(resp_buffer);

	if (rc != EOK) {
		usb_log_error("Failed to write multi registry value.\n");
		return rc;
	}

	return rc;
}

/** Send WMI message to HTC device.
 *
 * @param htc_device      HTC device structure.
 * @param command_id      Command identification.
 * @param command_buffer  Buffer with command data.
 * @param command_length  Length of command data.
 * @param response_buffer Buffer with response data.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t wmi_send_command(htc_device_t *htc_device, wmi_command_t command_id,
    uint8_t *command_buffer, uint32_t command_length, void *response_buffer)
{
	size_t header_size = sizeof(wmi_command_header_t) +
	    sizeof(htc_frame_header_t);
	size_t buffer_size = header_size + command_length;
	void *buffer = malloc(buffer_size);

	if (command_buffer != NULL)
		memcpy(buffer + header_size, command_buffer, command_length);

	/* Set up WMI header */
	wmi_command_header_t *wmi_header = (wmi_command_header_t *)
	    ((void *) buffer + sizeof(htc_frame_header_t));
	wmi_header->command_id = host2uint16_t_be(command_id);
	wmi_header->sequence_number =
	    host2uint16_t_be(++htc_device->sequence_number);

	/* Send message. */
	errno_t rc = htc_send_control_message(htc_device, buffer, buffer_size,
	    htc_device->endpoints.wmi_endpoint);
	if (rc != EOK) {
		free(buffer);
		usb_log_error("Failed to send WMI message. Error: %s\n", str_error_name(rc));
		return rc;
	}

	free(buffer);

	bool clean_resp_buffer = false;
	size_t response_buffer_size =
	    htc_device->ath_device->ctrl_response_length;
	if (response_buffer == NULL) {
		response_buffer = malloc(response_buffer_size);
		clean_resp_buffer = true;
	}

	/* Read response. */
	/* TODO: Ignoring WMI management RX messages ~ TX statuses etc. */
	uint16_t cmd_id;
	do {
		rc = htc_read_control_message(htc_device, response_buffer,
		    response_buffer_size, NULL);
		if (rc != EOK) {
			free(buffer);
			usb_log_error("Failed to receive WMI message response. "
			    "Error: %s\n", str_error_name(rc));
			return rc;
		}

		if (response_buffer_size < sizeof(htc_frame_header_t) +
		    sizeof(wmi_command_header_t)) {
			free(buffer);
			usb_log_error("Corrupted response received.\n");
			return EINVAL;
		}

		wmi_command_header_t *wmi_hdr = (wmi_command_header_t *)
		    ((void *) response_buffer + sizeof(htc_frame_header_t));
		cmd_id = uint16_t_be2host(wmi_hdr->command_id);
	} while (cmd_id & WMI_MGMT_CMD_MASK);

	if (clean_resp_buffer)
		free(response_buffer);

	return rc;
}
