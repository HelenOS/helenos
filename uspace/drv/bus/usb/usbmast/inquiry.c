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
#include <usb/dev/driver.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/massstor.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <ctype.h>
#include <scsi/spc.h>
#include "cmds.h"
#include "mast.h"

#define BITS_GET_MASK(type, bitcount) (((type)(1 << (bitcount)))-1)
#define BITS_GET_MID_MASK(type, bitcount, offset) \
	((type)( BITS_GET_MASK(type, (bitcount) + (offset)) - BITS_GET_MASK(type, bitcount) ))
#define BITS_GET(type, number, bitcount, offset) \
	((type)( (number) & (BITS_GET_MID_MASK(type, bitcount, offset)) ) >> (offset))

/** Get string representation for SCSI peripheral device type.
 *
 * @param type SCSI peripheral device type code.
 * @return String representation.
 */
const char *usb_str_masstor_scsi_peripheral_device_type(unsigned type)
{
	return scsi_get_dev_type_str(type);
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
	scsi_std_inquiry_data_t inq_data;
	size_t response_len;
	scsi_cdb_inquiry_t inquiry = {
		.op_code = SCSI_CMD_INQUIRY,
		.evpd = 0,
		.page_code = 0,
		.alloc_len = host2uint16_t_be(sizeof(inq_data)),
		.control = 0
	};

	int rc;

	rc = usb_massstor_data_in(dev, bulk_in_idx, bulk_out_idx,
	    0xDEADBEEF, 0, (uint8_t *) &inquiry, sizeof(inquiry),
	    &inq_data, sizeof(inq_data), &response_len);

	if (rc != EOK) {
		usb_log_error("Failed to probe device %s using %s: %s.\n",
		   dev->ddf_dev->name, "SCSI:INQUIRY", str_error(rc));
		return rc;
	}

	if (response_len < SCSI_STD_INQUIRY_DATA_MIN_SIZE) {
		usb_log_error("The SCSI inquiry response is too short.\n");
		return EIO;
	}

	/*
	 * Parse inquiry data and fill in the result structure.
	 */

	bzero(inquiry_result, sizeof(*inquiry_result));

	inquiry_result->device_type =
	    BITS_GET(uint8_t, inq_data.pqual_devtype, 5, 0);
	inquiry_result->removable =
	    BITS_GET(uint8_t, inq_data.rmb, 1, 7);

	str_ncpy(inquiry_result->vendor, 9,
	    (const char *) &inq_data.vendor, 8);
	trim_trailing_spaces(inquiry_result->vendor);

	str_ncpy(inquiry_result->product, 17,
	    (const char *) &inq_data.product, 16);
	trim_trailing_spaces(inquiry_result->product);

	str_ncpy(inquiry_result->revision, 5,
	    (const char *) &inq_data.revision, 4);
	trim_trailing_spaces(inquiry_result->revision);

	return EOK;
}

/**
 * @}
 */
