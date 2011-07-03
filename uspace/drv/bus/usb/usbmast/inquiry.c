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
#include <bitops.h>
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

/** Get string representation for SCSI peripheral device type.
 *
 * @param type SCSI peripheral device type code.
 * @return String representation.
 */
const char *usb_str_masstor_scsi_peripheral_device_type(unsigned type)
{
	return scsi_get_dev_type_str(type);
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

	rc = usb_massstor_data_in(dev, 0xDEADBEEF, 0, (uint8_t *) &inquiry,
	    sizeof(inquiry), &inq_data, sizeof(inq_data), &response_len);

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

	inquiry_result->device_type = BIT_RANGE_EXTRACT(uint8_t,
	    inq_data.pqual_devtype, SCSI_PQDT_DEV_TYPE_h, SCSI_PQDT_DEV_TYPE_l);

	inquiry_result->removable = BIT_RANGE_EXTRACT(uint8_t,
	    inq_data.rmb, SCSI_RMB_RMB, SCSI_RMB_RMB);

	spascii_to_str(inquiry_result->vendor, SCSI_INQ_VENDOR_STR_BUFSIZE,
	    inq_data.vendor, sizeof(inq_data.vendor));

	spascii_to_str(inquiry_result->product, SCSI_INQ_PRODUCT_STR_BUFSIZE,
	    inq_data.product, sizeof(inq_data.product));

	spascii_to_str(inquiry_result->revision, SCSI_INQ_REVISION_STR_BUFSIZE,
	    inq_data.revision, sizeof(inq_data.revision));

	return EOK;
}

/**
 * @}
 */
