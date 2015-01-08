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

#include <malloc.h>
#include <mem.h>
#include <byteorder.h>

#include "wmi.h"

/**
 * WMI registry read.
 * 
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be read.
 * @param res Stored result.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int wmi_reg_read(htc_device_t *htc_device, uint32_t reg_offset, uint32_t *res)
{
	uint32_t resp_value;
	uint32_t cmd_value = host2uint32_t_be(reg_offset);
	
	int rc = wmi_send_command(htc_device, WMI_REG_READ, 
		(uint8_t *) &cmd_value, sizeof(cmd_value),
		(uint8_t *) &resp_value, sizeof(resp_value));
	
	if(rc != EOK) {
		usb_log_error("Failed to read registry value.\n");
		return rc;
	}
	
	*res = uint32_t_be2host(resp_value);
	
	return rc;
}

/**
 * WMI registry write.
 * 
 * @param htc_device HTC device structure.
 * @param reg_offset Registry offset (address) to be written.
 * @param val Value to be written
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int wmi_reg_write(htc_device_t *htc_device, uint32_t reg_offset, uint32_t val)
{
	uint32_t resp_value = host2uint32_t_be(val);	
	uint32_t cmd_buffer[] = {
	    host2uint32_t_be(reg_offset),
	    resp_value
	};
	
	int rc = wmi_send_command(htc_device, WMI_REG_WRITE, 
		(uint8_t *) &cmd_buffer, sizeof(cmd_buffer),
		(uint8_t *) &resp_value, sizeof(resp_value));
	
	if(rc != EOK) {
		usb_log_error("Failed to write registry value.\n");
		return rc;
	}
	
	return rc;
}

/**
 * WMI multi registry write.
 * 
 * @param htc_device HTC device structure.
 * @param reg_buffer Array of registry values to be written.
 * @param elements Number of elements in array.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int wmi_reg_buffer_write(htc_device_t *htc_device, reg_buffer_t *reg_buffer,
	size_t elements)
{
	uint32_t resp_value;
	
	/* Convert values to correct endianness. */
	for(size_t i = 0; i < elements; i++) {
		reg_buffer_t *buffer_element = &reg_buffer[i];
		buffer_element->offset = 
			host2uint32_t_be(buffer_element->offset);
		buffer_element->value =
			host2uint32_t_be(buffer_element->value);
	}
	
	int rc = wmi_send_command(htc_device, WMI_REG_WRITE, 
		(uint8_t *) &reg_buffer, sizeof(reg_buffer_t)*elements,
		(uint8_t *) &resp_value, sizeof(resp_value));
	
	if(rc != EOK) {
		usb_log_error("Failed to write multi registry value.\n");
		return rc;
	}
	
	return rc;
}

/**
 * Send WMI message to HTC device.
 * 
 * @param htc_device HTC device structure.
 * @param command_id Command identification.
 * @param command_buffer Buffer with command data.
 * @param command_length Length of command data.
 * @param response_buffer Buffer with response data.
 * @param response_length Length of response data.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int wmi_send_command(htc_device_t *htc_device, wmi_command_t command_id, 
    uint8_t *command_buffer, uint32_t command_length, 
    uint8_t *response_buffer, uint32_t response_length) 
{
	size_t header_size = sizeof(wmi_command_header_t) + 
		sizeof(htc_frame_header_t);
	size_t buffer_size = header_size + command_length;
	void *buffer = malloc(buffer_size);
	memcpy(buffer+header_size, command_buffer, command_length);
	
	/* Set up WMI header */
	wmi_command_header_t *wmi_header = (wmi_command_header_t *)
		((void *) buffer + sizeof(htc_frame_header_t));
	wmi_header->command_id = 
		host2uint16_t_be(command_id);
	wmi_header->sequence_number = 
		host2uint16_t_be(++htc_device->sequence_number);
	
	/* Send message. */
	int rc = htc_send_message(htc_device, buffer, buffer_size,
		htc_device->endpoints.wmi_endpoint);
	if(rc != EOK) {
		free(buffer);
		usb_log_error("Failed to send WMI message. Error: %d\n", rc);
		return rc;
	}
	
	free(buffer);
	
	buffer_size = header_size + response_length;
	buffer = malloc(buffer_size);
	
	/* Read response. */
	rc = htc_read_message(htc_device, buffer, buffer_size, NULL);
	if(rc != EOK) {
		free(buffer);
		usb_log_error("Failed to receive WMI message response. "
		    "Error: %d\n", rc);
		return rc;
	}
	
	memcpy(response_buffer, 
		buffer + sizeof(wmi_command_header_t), 
		response_length);
	
	free(buffer);
	
	return rc;
}