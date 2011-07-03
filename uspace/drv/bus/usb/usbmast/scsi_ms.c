/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jiri Svoboda
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
 * SCSI functions for USB mass storage driver.
 */
#include <bitops.h>
#include <byteorder.h>
#include <inttypes.h>
#include <usb/dev/driver.h>
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <scsi/sbc.h>
#include <scsi/spc.h>
#include "cmds.h"
#include "mast.h"
#include "scsi_ms.h"

/** Get string representation for SCSI peripheral device type.
 *
 * @param type		SCSI peripheral device type code.
 * @return		String representation.
 */
const char *usbmast_scsi_dev_type_str(unsigned type)
{
	return scsi_get_dev_type_str(type);
}

/** Perform SCSI Inquiry command on USB mass storage device.
 *
 * @param dev		USB device.
 * @param inquiry_result Where to store parsed inquiry result.
 * @return		Error code.
 */
int usbmast_inquiry(usb_device_t *dev, usbmast_inquiry_data_t *inq_res)
{
	scsi_std_inquiry_data_t inq_data;
	size_t response_len;
	scsi_cdb_inquiry_t cdb;
	int rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_INQUIRY;
	cdb.alloc_len = host2uint16_t_be(sizeof(inq_data));

	rc = usb_massstor_data_in(dev, 0xDEADBEEF, 0, (uint8_t *) &cdb,
	    sizeof(cdb), &inq_data, sizeof(inq_data), &response_len);

	if (rc != EOK) {
		usb_log_error("Inquiry failed, device %s: %s.\n",
		   dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	if (response_len < SCSI_STD_INQUIRY_DATA_MIN_SIZE) {
		usb_log_error("SCSI Inquiry response too short (%zu).\n",
		    response_len);
		return EIO;
	}

	/*
	 * Parse inquiry data and fill in the result structure.
	 */

	bzero(inq_res, sizeof(*inq_res));

	inq_res->device_type = BIT_RANGE_EXTRACT(uint8_t,
	    inq_data.pqual_devtype, SCSI_PQDT_DEV_TYPE_h, SCSI_PQDT_DEV_TYPE_l);

	inq_res->removable = BIT_RANGE_EXTRACT(uint8_t,
	    inq_data.rmb, SCSI_RMB_RMB, SCSI_RMB_RMB);

	spascii_to_str(inq_res->vendor, SCSI_INQ_VENDOR_STR_BUFSIZE,
	    inq_data.vendor, sizeof(inq_data.vendor));

	spascii_to_str(inq_res->product, SCSI_INQ_PRODUCT_STR_BUFSIZE,
	    inq_data.product, sizeof(inq_data.product));

	spascii_to_str(inq_res->revision, SCSI_INQ_REVISION_STR_BUFSIZE,
	    inq_data.revision, sizeof(inq_data.revision));

	return EOK;
}

/** Perform SCSI Read Capacity command on USB mass storage device.
 *
 * @param dev		USB device.
 * @param nblocks	Output, number of blocks.
 * @param block_size	Output, block size in bytes.
 *
 * @return		Error code.
 */
int usbmast_read_capacity(usb_device_t *dev, uint32_t *nblocks,
    uint32_t *block_size)
{
	scsi_cdb_read_capacity_10_t cdb;
	scsi_read_capacity_10_data_t data;
	size_t data_len;
	int rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_READ_CAPACITY_10;

	rc = usb_massstor_data_in(dev, 0xDEADBEEF, 0, (uint8_t *) &cdb,
	    sizeof(cdb), &data, sizeof(data), &data_len);

        if (rc != EOK) {
		usb_log_error("Read Capacity (10) failed, device %s: %s.\n",
		   dev->ddf_dev->name, str_error(rc));
		return rc;
	}

	if (data_len < sizeof(data)) {
		usb_log_error("SCSI Read Capacity response too short (%zu).\n",
		    data_len);
		return EIO;
	}

	*nblocks = uint32_t_be2host(data.last_lba) + 1;
	*block_size = uint32_t_be2host(data.block_size);

	return EOK;
}

/**
 * @}
 */
