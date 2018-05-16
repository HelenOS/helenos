/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jiri Svoboda
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
#include <macros.h>
#include <usb/dev/driver.h>
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <scsi/sbc.h>
#include <scsi/spc.h>
#include "cmdw.h"
#include "bo_trans.h"
#include "scsi_ms.h"
#include "usbmast.h"

/** Get string representation for SCSI peripheral device type.
 *
 * @param type		SCSI peripheral device type code.
 * @return		String representation.
 */
const char *usbmast_scsi_dev_type_str(unsigned type)
{
	return scsi_get_dev_type_str(type);
}

static void usbmast_dump_sense(scsi_sense_data_t *sense_buf)
{
	const unsigned sense_key = sense_buf->flags_key & 0x0f;
	printf("Got sense data. Sense key: 0x%x (%s), ASC 0x%02x, "
	    "ASCQ 0x%02x.\n", sense_key,
	    scsi_get_sense_key_str(sense_key),
	    sense_buf->additional_code,
	    sense_buf->additional_cqual);
}

static errno_t usb_massstor_unit_ready(usbmast_fun_t *mfun)
{
	scsi_cmd_t cmd;
	scsi_cdb_test_unit_ready_t cdb;
	errno_t rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_TEST_UNIT_READY;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cdb = &cdb;
	cmd.cdb_size = sizeof(cdb);

	rc = usb_massstor_cmd(mfun, 0xDEADBEEF, &cmd);

	if (rc != EOK) {
		usb_log_error("Test Unit Ready failed on device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}
	/*
	 * Ignore command error here. If there's something wrong
	 * with the device the following commands will fail too.
	 */
	if (cmd.status != CMDS_GOOD)
		usb_log_warning("Test Unit Ready command failed on device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));

	return EOK;
}

/** Run SCSI command.
 *
 * Run command and repeat in case of unit attention.
 * XXX This is too simplified.
 */
static errno_t usbmast_run_cmd(usbmast_fun_t *mfun, scsi_cmd_t *cmd)
{
	uint8_t sense_key;
	scsi_sense_data_t sense_buf;
	errno_t rc;

	do {
		rc = usb_massstor_unit_ready(mfun);
		if (rc != EOK) {
			usb_log_error("Inquiry transport failed, device %s: %s.",
			    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
			return rc;
		}

		rc = usb_massstor_cmd(mfun, 0xDEADBEEF, cmd);
		if (rc != EOK) {
			usb_log_error("Inquiry transport failed, device %s: %s.",
			    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
			return rc;
		}

		if (cmd->status == CMDS_GOOD)
			return EOK;

		usb_log_error("SCSI command failed, device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));

		rc = usbmast_request_sense(mfun, &sense_buf, sizeof(sense_buf));
		if (rc != EOK) {
			usb_log_error("Failed to read sense data.");
			return EIO;
		}

		/* Dump sense data to log */
		usbmast_dump_sense(&sense_buf);

		/* Get sense key */
		sense_key = sense_buf.flags_key & 0x0f;

		if (sense_key == SCSI_SK_UNIT_ATTENTION) {
			printf("Got unit attention. Re-trying command.\n");
		}

	} while (sense_key == SCSI_SK_UNIT_ATTENTION);

	/* Command status is not good, nevertheless transport succeeded. */
	return EOK;
}

/** Perform SCSI Inquiry command on USB mass storage device.
 *
 * @param mfun		Mass storage function
 * @param inquiry_result Where to store parsed inquiry result
 * @return		Error code
 */
errno_t usbmast_inquiry(usbmast_fun_t *mfun, usbmast_inquiry_data_t *inq_res)
{
	scsi_std_inquiry_data_t inq_data;
	scsi_cmd_t cmd;
	scsi_cdb_inquiry_t cdb;
	errno_t rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_INQUIRY;
	cdb.alloc_len = host2uint16_t_be(sizeof(inq_data));

	memset(&cmd, 0, sizeof(cmd));
	cmd.cdb = &cdb;
	cmd.cdb_size = sizeof(cdb);
	cmd.data_in = &inq_data;
	cmd.data_in_size = sizeof(inq_data);

	rc = usb_massstor_cmd(mfun, 0xDEADBEEF, &cmd);

	if (rc != EOK) {
		usb_log_error("Inquiry transport failed, device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}

	if (cmd.status != CMDS_GOOD) {
		usb_log_error("Inquiry command failed, device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));
		return EIO;
	}

	if (cmd.rcvd_size < SCSI_STD_INQUIRY_DATA_MIN_SIZE) {
		usb_log_error("SCSI Inquiry response too short (%zu).",
		    cmd.rcvd_size);
		return EIO;
	}

	/*
	 * Parse inquiry data and fill in the result structure.
	 */

	memset(inq_res, 0, sizeof(*inq_res));

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

/** Perform SCSI Request Sense command on USB mass storage device.
 *
 * @param mfun		Mass storage function
 * @param buf		Destination buffer
 * @param size		Size of @a buf
 *
 * @return		Error code.
 */
errno_t usbmast_request_sense(usbmast_fun_t *mfun, void *buf, size_t size)
{
	scsi_cmd_t cmd;
	scsi_cdb_request_sense_t cdb;
	errno_t rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_REQUEST_SENSE;
	cdb.alloc_len = min(size, SCSI_SENSE_DATA_MAX_SIZE);

	memset(&cmd, 0, sizeof(cmd));
	cmd.cdb = &cdb;
	cmd.cdb_size = sizeof(cdb);
	cmd.data_in = buf;
	cmd.data_in_size = size;

	rc = usb_massstor_cmd(mfun, 0xDEADBEEF, &cmd);

	if (rc != EOK || cmd.status != CMDS_GOOD) {
		usb_log_error("Request Sense failed, device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}

	if (cmd.rcvd_size < SCSI_SENSE_DATA_MIN_SIZE) {
		/* The missing bytes should be considered to be zeroes. */
		memset((uint8_t *)buf + cmd.rcvd_size, 0,
		    SCSI_SENSE_DATA_MIN_SIZE - cmd.rcvd_size);
	}

	return EOK;
}

/** Perform SCSI Read Capacity command on USB mass storage device.
 *
 * @param mfun		Mass storage function
 * @param nblocks	Output, number of blocks
 * @param block_size	Output, block size in bytes
 *
 * @return		Error code.
 */
errno_t usbmast_read_capacity(usbmast_fun_t *mfun, uint32_t *nblocks,
    uint32_t *block_size)
{
	scsi_cmd_t cmd;
	scsi_cdb_read_capacity_10_t cdb;
	scsi_read_capacity_10_data_t data;
	errno_t rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_READ_CAPACITY_10;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cdb = &cdb;
	cmd.cdb_size = sizeof(cdb);
	cmd.data_in = &data;
	cmd.data_in_size = sizeof(data);

	rc = usbmast_run_cmd(mfun, &cmd);

	if (rc != EOK) {
		usb_log_error("Read Capacity (10) transport failed, device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}

	if (cmd.status != CMDS_GOOD) {
		usb_log_error("Read Capacity (10) command failed, device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));
		return EIO;
	}

	if (cmd.rcvd_size < sizeof(data)) {
		usb_log_error("SCSI Read Capacity response too short (%zu).",
		    cmd.rcvd_size);
		return EIO;
	}

	*nblocks = uint32_t_be2host(data.last_lba) + 1;
	*block_size = uint32_t_be2host(data.block_size);

	return EOK;
}

/** Perform SCSI Read command on USB mass storage device.
 *
 * @param mfun		Mass storage function
 * @param ba		Address of first block
 * @param nblocks	Number of blocks to read
 *
 * @return		Error code
 */
errno_t usbmast_read(usbmast_fun_t *mfun, uint64_t ba, size_t nblocks, void *buf)
{
	scsi_cmd_t cmd;
	scsi_cdb_read_10_t cdb;
	errno_t rc;

	if (ba > UINT32_MAX)
		return ELIMIT;

	if (nblocks > UINT16_MAX)
		return ELIMIT;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_READ_10;
	cdb.lba = host2uint32_t_be(ba);
	cdb.xfer_len = host2uint16_t_be(nblocks);

	memset(&cmd, 0, sizeof(cmd));
	cmd.cdb = &cdb;
	cmd.cdb_size = sizeof(cdb);
	cmd.data_in = buf;
	cmd.data_in_size = nblocks * mfun->block_size;

	rc = usbmast_run_cmd(mfun, &cmd);

	if (rc != EOK) {
		usb_log_error("Read (10) transport failed, device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}

	if (cmd.status != CMDS_GOOD) {
		usb_log_error("Read (10) command failed, device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));
		return EIO;
	}

	if (cmd.rcvd_size < nblocks * mfun->block_size) {
		usb_log_error("SCSI Read response too short (%zu).",
		    cmd.rcvd_size);
		return EIO;
	}

	return EOK;
}

/** Perform SCSI Write command on USB mass storage device.
 *
 * @param mfun		Mass storage function
 * @param ba		Address of first block
 * @param nblocks	Number of blocks to read
 * @param data		Data to write
 *
 * @return		Error code
 */
errno_t usbmast_write(usbmast_fun_t *mfun, uint64_t ba, size_t nblocks,
    const void *data)
{
	scsi_cmd_t cmd;
	scsi_cdb_write_10_t cdb;
	errno_t rc;

	if (ba > UINT32_MAX)
		return ELIMIT;

	if (nblocks > UINT16_MAX)
		return ELIMIT;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_WRITE_10;
	cdb.lba = host2uint32_t_be(ba);
	cdb.xfer_len = host2uint16_t_be(nblocks);

	memset(&cmd, 0, sizeof(cmd));
	cmd.cdb = &cdb;
	cmd.cdb_size = sizeof(cdb);
	cmd.data_out = data;
	cmd.data_out_size = nblocks * mfun->block_size;

	rc = usbmast_run_cmd(mfun, &cmd);

	if (rc != EOK) {
		usb_log_error("Write (10) transport failed, device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}

	if (cmd.status != CMDS_GOOD) {
		usb_log_error("Write (10) command failed, device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));
		return EIO;
	}

	return EOK;
}

/** Perform SCSI Synchronize Cache command on USB mass storage device.
 *
 * @param mfun		Mass storage function
 * @param ba		Address of first block
 * @param nblocks	Number of blocks to read
 * @param data		Data to write
 *
 * @return		Error code
 */
errno_t usbmast_sync_cache(usbmast_fun_t *mfun, uint64_t ba, size_t nblocks)
{
	if (ba > UINT32_MAX)
		return ELIMIT;

	if (nblocks > UINT16_MAX)
		return ELIMIT;

	const scsi_cdb_sync_cache_10_t cdb = {
		.op_code = SCSI_CMD_SYNC_CACHE_10,
		.lba = host2uint32_t_be(ba),
		.numlb = host2uint16_t_be(nblocks),
	};

	scsi_cmd_t cmd = {
		.cdb = &cdb,
		.cdb_size = sizeof(cdb),
	};

	const errno_t rc = usbmast_run_cmd(mfun, &cmd);

	if (rc != EOK) {
		usb_log_error("Synchronize Cache (10) transport failed, device %s: %s.",
		    usb_device_get_name(mfun->mdev->usb_dev), str_error(rc));
		return rc;
	}

	if (cmd.status != CMDS_GOOD) {
		usb_log_error("Synchronize Cache (10) command failed, device %s.",
		    usb_device_get_name(mfun->mdev->usb_dev));
		return EIO;
	}

	return EOK;
}

/**
 * @}
 */
