/*
 * Copyright (c) 2010 Lubos Slovak
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
#include <usb/classes/hid.h>

#include "descdump.h"

/*----------------------------------------------------------------------------*/

#define BYTES_PER_LINE 12

static void dump_buffer(const char *msg, const uint8_t *buffer, size_t length)
{
	printf("%s\n", msg);

	size_t i;
	for (i = 0; i < length; i++) {
		printf("  0x%02X", buffer[i]);
		if (((i > 0) && (((i+1) % BYTES_PER_LINE) == 0))
		    || (i + 1 == length)) {
			printf("\n");
		}
	}
}

/*----------------------------------------------------------------------------*/

#define INDENT "  "

void dump_standard_configuration_descriptor(
    int index, const usb_standard_configuration_descriptor_t *d)
{
	bool self_powered = d->attributes & 64;
	bool remote_wakeup = d->attributes & 32;
	
	printf("Standard configuration descriptor #%d\n", index);
	printf(INDENT "bLength = %d\n", d->length);
	printf(INDENT "bDescriptorType = 0x%02x\n", d->descriptor_type);
	printf(INDENT "wTotalLength = %d\n", d->total_length);
	printf(INDENT "bNumInterfaces = %d\n", d->interface_count);
	printf(INDENT "bConfigurationValue = %d\n", d->configuration_number);
	printf(INDENT "iConfiguration = %d\n", d->str_configuration);
	printf(INDENT "bmAttributes = %d [%s%s%s]\n", d->attributes,
	    self_powered ? "self-powered" : "",
	    (self_powered & remote_wakeup) ? ", " : "",
	    remote_wakeup ? "remote-wakeup" : "");
	printf(INDENT "MaxPower = %d (%dmA)\n", d->max_power,
	    2 * d->max_power);
	// printf(INDENT " = %d\n", d->);
}

void dump_standard_interface_descriptor(
    const usb_standard_interface_descriptor_t *d)
{
	printf("Standard interface descriptor\n");
	printf(INDENT "bLength = %d\n", d->length);
	printf(INDENT "bDescriptorType = 0x%02x\n", d->descriptor_type);
	printf(INDENT "bInterfaceNumber = %d\n", d->interface_number);
	printf(INDENT "bAlternateSetting = %d\n", d->alternate_setting);
	printf(INDENT "bNumEndpoints = %d\n", d->endpoint_count);
	printf(INDENT "bInterfaceClass = %d\n", d->interface_class);
	printf(INDENT "bInterfaceSubClass = %d\n", d->interface_subclass);
	printf(INDENT "bInterfaceProtocol = %d\n", d->interface_protocol);
	printf(INDENT "iInterface = %d\n", d->str_interface);
}

void dump_standard_endpoint_descriptor(
    const usb_standard_endpoint_descriptor_t *d)
{
	const char *transfer_type;
	switch (d->attributes & 3) {
	case USB_TRANSFER_CONTROL:
		transfer_type = "control";
		break;
	case USB_TRANSFER_ISOCHRONOUS:
		transfer_type = "isochronous";
		break;
	case USB_TRANSFER_BULK:
		transfer_type = "bulk";
		break;
	case USB_TRANSFER_INTERRUPT:
		transfer_type = "interrupt";
		break;
	}

	printf("Standard endpoint descriptor\n");
	printf(INDENT "bLength = %d\n", d->length);
	printf(INDENT "bDescriptorType = 0x%02x\n", d->descriptor_type);
	printf(INDENT "bmAttributes = %d [%s]\n", d->attributes, transfer_type);
	printf(INDENT "wMaxPacketSize = %d\n", d->max_packet_size);
	printf(INDENT "bInterval = %d\n", d->poll_interval);
}

void dump_standard_hid_descriptor_header(
    const usb_standard_hid_descriptor_t *d)
{
	printf("Standard HID descriptor\n");
	printf(INDENT "bLength = %d\n", d->length);
	printf(INDENT "bDescriptorType = 0x%02x\n", d->descriptor_type);
	printf(INDENT "bcdHID = %d\n", d->spec_release);
	printf(INDENT "bCountryCode = %d\n", d->country_code);
	printf(INDENT "bNumDescriptors = %d\n", d->class_desc_count);
	printf(INDENT "bDescriptorType = %d\n", d->report_desc_info.type);
	printf(INDENT "wDescriptorLength = %d\n", d->report_desc_info.length);
}

void dump_standard_hid_class_descriptor_info(
    const usb_standard_hid_class_descriptor_info_t *d)
{
	printf(INDENT "bDescriptorType = %d\n", d->type);
	printf(INDENT "wDescriptorLength = %d\n", d->length);
}

void dump_hid_class_descriptor(int index, uint8_t type, 
    const uint8_t *d, size_t size )
{
	printf("Class-specific descriptor #%d (type: %u)\n", index, type);
	assert(d != NULL);
	dump_buffer("", d, size);
}
