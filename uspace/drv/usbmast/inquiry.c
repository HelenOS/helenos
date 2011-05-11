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

/** @addtogroup drvusbmast
 * @{
 */
/**
 * @file
 * Main routines of USB mass storage driver.
 */
#include <usb/devdrv.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/massstor.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <ctype.h>
#include "cmds.h"
#include "scsi.h"
#include "mast.h"

#define BITS_GET_MASK(type, bitcount) (((type)(1 << (bitcount)))-1)
#define BITS_GET_MID_MASK(type, bitcount, offset) \
	((type)( BITS_GET_MASK(type, (bitcount) + (offset)) - BITS_GET_MASK(type, bitcount) ))
#define BITS_GET(type, number, bitcount, offset) \
	((type)( (number) & (BITS_GET_MID_MASK(type, bitcount, offset)) ) >> (offset))

#define INQUIRY_RESPONSE_LENGTH 36

#define STR_UNKNOWN "<unknown>"

/** String constants for SCSI peripheral device types. */
static const char *str_peripheral_device_types[] = {
	"direct-access device",
	"sequential-access device",
	"printer device",
	"processor device",
	"write-once device",
	"CDROM device",
	"scanner device",
	"optical memory device",
	"medium changer",
	"communications device",
	"graphic arts pre-press device",
	"graphic arts pre-press device",
	"storage array controller device",
	"enclosure services device",
	"simplified direct-access device",
	"optical card reader/writer device",
	"bridging expander",
	"object-based storage device",
	"automation driver interface",
	STR_UNKNOWN, // 0x13
	STR_UNKNOWN, // 0x14
	STR_UNKNOWN, // 0x15
	STR_UNKNOWN, // 0x16
	STR_UNKNOWN, // 0x17
	STR_UNKNOWN, // 0x18
	STR_UNKNOWN, // 0x19
	STR_UNKNOWN, // 0x1A
	STR_UNKNOWN, // 0x1B
	STR_UNKNOWN, // 0x1C
	STR_UNKNOWN, // 0x1D
	"well-known logical unit",
	"uknown or no device state"
};
#define str_peripheral_device_types_count \
	(sizeof(str_peripheral_device_types)/sizeof(str_peripheral_device_types[0]))

/** Get string representation for SCSI peripheral device type.
 *
 * See for example here for a list
 * http://en.wikipedia.org/wiki/SCSI_Peripheral_Device_Type.
 *
 * @param type SCSI peripheral device type code.
 * @return String representation.
 */
const char *usb_str_masstor_scsi_peripheral_device_type(int type)
{
	if ((type < 0)
	    || ((size_t)type >= str_peripheral_device_types_count)) {
		return STR_UNKNOWN;
	}
	return str_peripheral_device_types[type];
}

/** Trim trailing spaces from a string (rewrite with string terminator).
 *
 * @param name String to be trimmed (in-out parameter).
 */
static void trim_trailing_spaces(char *name)
{
	size_t len = str_length(name);
	while ((len > 0) && isspace((int) name[len - 1])) {
		name[len - 1] = 0;
		len--;
	}
}

/** Perform SCSI INQUIRY command on USB mass storage device.
 *
 * @param dev USB device.
 * @param bulk_in_idx Index (in dev->pipes) of bulk in pipe.
 * @param bulk_out_idx Index of bulk out pipe.
 * @param inquiry_result Where to store parsed inquiry result.
 * @return Error code.
 */
int usb_massstor_inquiry(usb_device_t *dev,
    size_t bulk_in_idx, size_t bulk_out_idx,
    usb_massstor_inquiry_result_t *inquiry_result)
{
	scsi_cmd_inquiry_t inquiry = {
		.op_code = 0x12,
		.lun_evpd = 0,
		.page_code = 0,
		.alloc_length = host2uint16_t_be(INQUIRY_RESPONSE_LENGTH),
		.ctrl = 0
	};
	size_t response_len;
	uint8_t response[INQUIRY_RESPONSE_LENGTH];

	int rc;

	rc = usb_massstor_data_in(dev, bulk_in_idx, bulk_out_idx,
	    0xDEADBEEF, 0, (uint8_t *) &inquiry, sizeof(inquiry),
	    response, INQUIRY_RESPONSE_LENGTH, &response_len);

	if (rc != EOK) {
		usb_log_error("Failed to probe device %s using %s: %s.\n",
		   dev->ddf_dev->name, "SCSI:INQUIRY", str_error(rc));
		return rc;
	}

	if (response_len < 8) {
		usb_log_error("The SCSI response is too short.\n");
		return ERANGE;
	}

	/*
	 * This is an ugly part of the code. We will parse the returned
	 * data by hand and try to get as many useful data as possible.
	 */
	bzero(inquiry_result, sizeof(*inquiry_result));

	/* This shall be returned by all devices. */
	inquiry_result->peripheral_device_type
	    = BITS_GET(uint8_t, response[0], 5, 0);
	inquiry_result->removable = BITS_GET(uint8_t, response[1], 1, 7);

	if (response_len < 32) {
		return EOK;
	}

	str_ncpy(inquiry_result->vendor_id, 9,
	    (const char *) &response[8], 8);
	trim_trailing_spaces(inquiry_result->vendor_id);

	str_ncpy(inquiry_result->product_and_revision, 12,
	    (const char *) &response[16], 11);
	trim_trailing_spaces(inquiry_result->product_and_revision);

	return EOK;
}

/**
 * @}
 */
