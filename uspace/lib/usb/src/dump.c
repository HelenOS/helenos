/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2018 Ondrej Hlavaty
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
 * Descriptor dumping.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/classes/classes.h>
#include <usb/classes/hub.h>
#include <usb/usb.h>

/** Mapping between descriptor id and dumping function. */
typedef struct {
	/** Descriptor id. */
	int id;
	/** Dumping function. */
	void (*dump)(FILE *, const char *, const char *,
	    const uint8_t *, size_t);
} descriptor_dump_t;

static void usb_dump_descriptor_device(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_configuration(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_interface(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_string(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_endpoint(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_superspeed_endpoint_companion(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_hid(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_hub(FILE *, const char *, const char *,
    const uint8_t *, size_t);
static void usb_dump_descriptor_generic(FILE *, const char *, const char *,
    const uint8_t *, size_t);

/** Descriptor dumpers mapping. */
static descriptor_dump_t descriptor_dumpers[] = {
	{ USB_DESCTYPE_DEVICE, usb_dump_descriptor_device },
	{ USB_DESCTYPE_CONFIGURATION, usb_dump_descriptor_configuration },
	{ USB_DESCTYPE_STRING, usb_dump_descriptor_string },
	{ USB_DESCTYPE_INTERFACE, usb_dump_descriptor_interface },
	{ USB_DESCTYPE_ENDPOINT, usb_dump_descriptor_endpoint },
	{ USB_DESCTYPE_SSPEED_EP_COMPANION, usb_dump_descriptor_superspeed_endpoint_companion },
	{ USB_DESCTYPE_HID, usb_dump_descriptor_hid },
	{ USB_DESCTYPE_HUB, usb_dump_descriptor_hub },
	{ -1, usb_dump_descriptor_generic },
	{ -1, NULL }
};

/** Dumps standard USB descriptor.
 * The @p line_suffix must contain the newline <code>\\n</code> character.
 * When @p line_suffix or @p line_prefix is NULL, they are substitued with
 * default values
 * (<code> - </code> for prefix and line termination for suffix).
 *
 * @param output Output file stream to dump descriptor to.
 * @param line_prefix Prefix for each line of output.
 * @param line_suffix Suffix of each line of output.
 * @param descriptor Actual descriptor.
 * @param descriptor_length Descriptor size.
 */
void usb_dump_standard_descriptor(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	if (descriptor_length < 2) {
		return;
	}
	int type = descriptor[1];

	descriptor_dump_t *dumper = descriptor_dumpers;
	while (dumper->dump != NULL) {
		if ((dumper->id == type) || (dumper->id < 0)) {
			dumper->dump(output, line_prefix, line_suffix,
			    descriptor, descriptor_length);
			return;
		}
		dumper++;
	}
}

/** Prints single line of USB descriptor dump.
 * @warning This macro abuses heavily the naming conventions used
 * by all dumping functions (i.e. names for output file stream (@c output) and
 * line prefix and suffix (@c line_prefix and @c line_suffix respectively))-
 *
 * @param fmt Formatting string.
 */
#define PRINTLINE(fmt, ...) \
	fprintf(output, "%s" fmt "%s", \
	    line_prefix ? line_prefix : " - ", \
	    __VA_ARGS__, \
	    line_suffix ? line_suffix : "\n")

#define BCD_INT(a) (((unsigned int)(a)) / 256)
#define BCD_FRAC(a) (((unsigned int)(a)) % 256)

#define BCD_FMT "%x.%x"
#define BCD_ARGS(a) BCD_INT((a)), BCD_FRAC((a))

static void usb_dump_descriptor_device(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_standard_device_descriptor_t *d =
	    (usb_standard_device_descriptor_t *) descriptor;
	if (descriptor_length < sizeof(*d)) {
		return;
	}

	PRINTLINE("bLength = %d", d->length);
	PRINTLINE("bDescriptorType = 0x%02x", d->descriptor_type);
	PRINTLINE("bcdUSB = %d (" BCD_FMT ")", d->usb_spec_version,
	    BCD_ARGS(d->usb_spec_version));
	PRINTLINE("bDeviceClass = 0x%02x", d->device_class);
	PRINTLINE("bDeviceSubClass = 0x%02x", d->device_subclass);
	PRINTLINE("bDeviceProtocol = 0x%02x", d->device_protocol);
	PRINTLINE("bMaxPacketSize0 = %d", d->max_packet_size);
	PRINTLINE("idVendor = 0x%04x", d->vendor_id);
	PRINTLINE("idProduct = 0x%04x", d->product_id);
	PRINTLINE("bcdDevice = %d", d->device_version);
	PRINTLINE("iManufacturer = %d", d->str_manufacturer);
	PRINTLINE("iProduct = %d", d->str_product);
	PRINTLINE("iSerialNumber = %d", d->str_serial_number);
	PRINTLINE("bNumConfigurations = %d", d->configuration_count);
}

static void usb_dump_descriptor_configuration(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_standard_configuration_descriptor_t *d =
	    (usb_standard_configuration_descriptor_t *) descriptor;
	if (descriptor_length < sizeof(*d)) {
		return;
	}

	bool self_powered = d->attributes & 64;
	bool remote_wakeup = d->attributes & 32;

	PRINTLINE("bLength = %d", d->length);
	PRINTLINE("bDescriptorType = 0x%02x", d->descriptor_type);
	PRINTLINE("wTotalLength = %d", d->total_length);
	PRINTLINE("bNumInterfaces = %d", d->interface_count);
	PRINTLINE("bConfigurationValue = %d", d->configuration_number);
	PRINTLINE("iConfiguration = %d", d->str_configuration);
	PRINTLINE("bmAttributes = %d [%s%s%s]", d->attributes,
	    self_powered ? "self-powered" : "",
	    (self_powered & remote_wakeup) ? ", " : "",
	    remote_wakeup ? "remote-wakeup" : "");
	PRINTLINE("MaxPower = %d (%dmA)", d->max_power,
	    2 * d->max_power);
}

static void usb_dump_descriptor_interface(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_standard_interface_descriptor_t *d =
	    (usb_standard_interface_descriptor_t *) descriptor;
	if (descriptor_length < sizeof(*d)) {
		return;
	}

	PRINTLINE("bLength = %d", d->length);
	PRINTLINE("bDescriptorType = 0x%02x", d->descriptor_type);
	PRINTLINE("bInterfaceNumber = %d", d->interface_number);
	PRINTLINE("bAlternateSetting = %d", d->alternate_setting);
	PRINTLINE("bNumEndpoints = %d", d->endpoint_count);
	PRINTLINE("bInterfaceClass = %s", d->interface_class == 0 ?
	    "reserved (0)" : usb_str_class(d->interface_class));
	PRINTLINE("bInterfaceSubClass = %d", d->interface_subclass);
	PRINTLINE("bInterfaceProtocol = %d", d->interface_protocol);
	PRINTLINE("iInterface = %d", d->str_interface);
}

static void usb_dump_descriptor_string(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
}

static void usb_dump_descriptor_endpoint(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_standard_endpoint_descriptor_t *d =
	    (usb_standard_endpoint_descriptor_t *) descriptor;
	if (descriptor_length < sizeof(*d)) {
		return;
	}

	int endpoint = d->endpoint_address & 15;
	usb_direction_t direction = d->endpoint_address & 128 ?
	    USB_DIRECTION_IN : USB_DIRECTION_OUT;
	usb_transfer_type_t transfer_type = d->attributes & 3;

	PRINTLINE("bLength = %d", d->length);
	PRINTLINE("bDescriptorType = 0x%02X", d->descriptor_type);
	PRINTLINE("bEndpointAddress = 0x%02X [%d, %s]",
	    d->endpoint_address, endpoint,
	    direction == USB_DIRECTION_IN ? "in" : "out");
	PRINTLINE("bmAttributes = %d [%s]", d->attributes,
	    usb_str_transfer_type(transfer_type));
	PRINTLINE("wMaxPacketSize = %d", d->max_packet_size);
	PRINTLINE("bInterval = %dms", d->poll_interval);
}

static void usb_dump_descriptor_superspeed_endpoint_companion(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_superspeed_endpoint_companion_descriptor_t *d =
	    (usb_superspeed_endpoint_companion_descriptor_t *) descriptor;
	if (descriptor_length < sizeof(*d)) {
		return;
	}

	PRINTLINE("bLength = %u", d->length);
	PRINTLINE("bDescriptorType = 0x%02X", d->descriptor_type);
	PRINTLINE("bMaxBurst = %u", d->max_burst);
	PRINTLINE("bmAttributes = %d", d->attributes);
	PRINTLINE("wBytesPerInterval = %u", d->bytes_per_interval);
}

static void usb_dump_descriptor_hid(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_standard_hid_descriptor_t *d =
	    (usb_standard_hid_descriptor_t *) descriptor;
	if (descriptor_length < sizeof(*d)) {
		return;
	}

	PRINTLINE("bLength = %d", d->length);
	PRINTLINE("bDescriptorType = 0x%02x", d->descriptor_type);
	PRINTLINE("bcdHID = %d (" BCD_FMT ")", d->spec_release,
	    BCD_ARGS(d->spec_release));
	PRINTLINE("bCountryCode = %d", d->country_code);
	PRINTLINE("bNumDescriptors = %d", d->class_desc_count);
	PRINTLINE("bDescriptorType = %d", d->report_desc_info.type);
	PRINTLINE("wDescriptorLength = %d", d->report_desc_info.length);

	/* Print info about report descriptors. */
	size_t i;
	size_t count = (descriptor_length - sizeof(*d)) /
	    sizeof(usb_standard_hid_class_descriptor_info_t);
	usb_standard_hid_class_descriptor_info_t *d2 =
	    (usb_standard_hid_class_descriptor_info_t *)
	    (descriptor + sizeof(*d));
	for (i = 0; i < count; i++, d2++) {
		PRINTLINE("bDescriptorType = %d", d2->type);
		PRINTLINE("wDescriptorLength = %d", d2->length);
	}
}

static void usb_dump_descriptor_hub(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	usb_hub_descriptor_header_t *d =
	    (usb_hub_descriptor_header_t *) descriptor;
	if (descriptor_length < sizeof(d))
		return;

	PRINTLINE("bDescLength: = %d", d->length);
	PRINTLINE("bDescriptorType = 0x%02x", d->descriptor_type);
	PRINTLINE("bNbrPorts = %d", d->port_count);
	PRINTLINE("bHubCharacteristics = 0x%02x%02x (%s;%s%s)",
	    d->characteristics_reserved, d->characteristics,
	    (d->characteristics & HUB_CHAR_NO_POWER_SWITCH_FLAG) ?
	    "No Power Switching" :
	    ((d->characteristics & HUB_CHAR_POWER_PER_PORT_FLAG) ?
	    "Per-Port Switching" : "Ganged Power Switching"),
	    (d->characteristics & HUB_CHAR_COMPOUND_DEVICE) ?
	    "Compound Device;" : "",
	    (d->characteristics & HUB_CHAR_NO_OC_FLAG) ?
	    "No OC Protection" :
	    ((d->characteristics & HUB_CHAR_OC_PER_PORT_FLAG) ?
	    "Individual Port OC Protection" :
	    "Global OC Protection"));
	PRINTLINE("bPwrOn2PwrGood = %d (%d ms)",
	    d->power_good_time, d->power_good_time * 2);
	PRINTLINE("bHubContrCurrent = %d (%d mA)",
	    d->max_current, d->max_current);
	const size_t port_bytes = (descriptor_length - sizeof(*d)) / 2;
	const uint8_t *removable_mask = descriptor + sizeof(*d);
	const uint8_t *powered_mask = descriptor + sizeof(*d) + port_bytes;

	if (port_bytes == 0 ||
	    port_bytes > (((d->port_count / (unsigned)8) + 1) * 2)) {
		PRINTLINE("::CORRUPTED DESCRIPTOR:: (%zu bytes remain)",
		    port_bytes * 2);
	}

	fprintf(output, "%sDeviceRemovable = 0x",
	    line_prefix ? line_prefix : " - ");
	for (unsigned i = port_bytes; i > 0; --i)
		fprintf(output, "%02x", removable_mask[i - 1]);
	fprintf(output, " (0b1 - Device non-removable)%s",
	    line_suffix ? line_suffix : "\n");

	fprintf(output, "%sPortPwrCtrlMask = 0x",
	    line_prefix ? line_prefix : " - ");
	for (unsigned i = port_bytes; i > 0; --i)
		fprintf(output, "%02x", powered_mask[i - 1]);
	fprintf(output, " (Legacy - All should be 0b1)%s",
	    line_suffix ? line_suffix : "\n");
}

static void usb_dump_descriptor_generic(FILE *output,
    const char *line_prefix, const char *line_suffix,
    const uint8_t *descriptor, size_t descriptor_length)
{
	/* TODO */
}

/**
 * @}
 */
