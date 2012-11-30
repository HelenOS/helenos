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
 * USB mass storage bulk-only transport.
 */
#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/dev/request.h>

#include "bo_trans.h"
#include "cmdw.h"
#include "usbmast.h"

bool usb_mast_verbose = false;

#define MASTLOG(format, ...) \
	do { \
		if (usb_mast_verbose) { \
			usb_log_debug2("USB cl08: " format, ##__VA_ARGS__); \
		} \
	} while (false)

/** Send command via bulk-only transport.
 *
 * @param mfun		Mass storage function
 * @param tag		Command block wrapper tag (automatically compared
 *			with answer)
 * @param cmd		SCSI command
 *
 * @return		Error code
 */
int usb_massstor_cmd(usbmast_fun_t *mfun, uint32_t tag, scsi_cmd_t *cmd)
{
	int rc;
	int retval = EOK;
	size_t act_size;
	usb_pipe_t *bulk_in_pipe = &mfun->mdev->usb_dev->pipes[BULK_IN_EP].pipe;
	usb_pipe_t *bulk_out_pipe = &mfun->mdev->usb_dev->pipes[BULK_OUT_EP].pipe;
	usb_direction_t ddir;
	void *dbuf;
	size_t dbuf_size;

	if (cmd->data_out != NULL && cmd->data_in == NULL) {
		ddir = USB_DIRECTION_OUT;
		dbuf = (void *)cmd->data_out;
		dbuf_size = cmd->data_out_size;
	} else if (cmd->data_out == NULL && cmd->data_in != NULL) {
		ddir = USB_DIRECTION_IN;
		dbuf = cmd->data_in;
		dbuf_size = cmd->data_in_size;
	} else {
		assert(false);
	}

	/* Prepare CBW - command block wrapper */
	usb_massstor_cbw_t cbw;
	usb_massstor_cbw_prepare(&cbw, tag, dbuf_size, ddir, mfun->lun,
	    cmd->cdb_size, cmd->cdb);

	/* Send the CBW. */
	MASTLOG("Sending CBW.\n");
	rc = usb_pipe_write(bulk_out_pipe, &cbw, sizeof(cbw));
	MASTLOG("CBW '%s' sent: %s.\n",
	    usb_debug_str_buffer((uint8_t *) &cbw, sizeof(cbw), 0),
	    str_error(rc));
	if (rc != EOK)
		return EIO;

	MASTLOG("Transferring data.\n");
	if (ddir == USB_DIRECTION_IN) {
		/* Recieve data from the device. */
		rc = usb_pipe_read(bulk_in_pipe, dbuf, dbuf_size, &act_size);
		MASTLOG("Received %zu bytes (%s): %s.\n", act_size,
		    usb_debug_str_buffer((uint8_t *) dbuf, act_size, 0),
		    str_error(rc));
	} else {
		/* Send data to the device. */
		rc = usb_pipe_write(bulk_out_pipe, dbuf, dbuf_size);
		MASTLOG("Sent %zu bytes (%s): %s.\n", act_size,
		    usb_debug_str_buffer((uint8_t *) dbuf, act_size, 0),
		    str_error(rc));
	}

	if (rc == ESTALL) {
		/* Clear stall condition and continue below to read CSW. */
		if (ddir == USB_DIRECTION_IN) {
			usb_pipe_clear_halt(&mfun->mdev->usb_dev->ctrl_pipe,
			    &mfun->mdev->usb_dev->pipes[BULK_IN_EP].pipe);
		} else {
			usb_pipe_clear_halt(&mfun->mdev->usb_dev->ctrl_pipe,
			    &mfun->mdev->usb_dev->pipes[BULK_OUT_EP].pipe);
		}
        } else if (rc != EOK) {
		return EIO;
	}

	/* Read CSW. */
	usb_massstor_csw_t csw;
	size_t csw_size;
	MASTLOG("Reading CSW.\n");
	rc = usb_pipe_read(bulk_in_pipe, &csw, sizeof(csw), &csw_size);
	MASTLOG("CSW '%s' received (%zu bytes): %s.\n",
	    usb_debug_str_buffer((uint8_t *) &csw, csw_size, 0), csw_size,
	    str_error(rc));
	if (rc != EOK) {
		MASTLOG("rc != EOK\n");
		return EIO;
	}

	if (csw_size != sizeof(csw)) {
		MASTLOG("csw_size != sizeof(csw)\n");
		return EIO;
	}

	if (csw.dCSWTag != tag) {
		MASTLOG("csw.dCSWTag != tag\n");
		return EIO;
	}

	/*
	 * Determine the actual return value from the CSW.
	 */
	switch (csw.dCSWStatus) {
	case cbs_passed:
		cmd->status = CMDS_GOOD;
		break;
	case cbs_failed:
		MASTLOG("Command failed\n");
		cmd->status = CMDS_FAILED;
		break;
	case cbs_phase_error:
		MASTLOG("Phase error\n");
		retval = EIO;
		break;
	default:
		retval = EIO;
		break;
	}

	size_t residue = (size_t) uint32_usb2host(csw.dCSWDataResidue);
	if (residue > dbuf_size) {
		MASTLOG("residue > dbuf_size\n");
		return EIO;
	}

	/*
	 * When the device has less data to send than requested (or cannot
	 * receive moredata), it can either stall the pipe or send garbage
	 * (ignore data) and indicate that via the residue field in CSW.
	 * That means dbuf_size - residue is the authoritative size of data
	 * received (sent).
	 */

	if (ddir == USB_DIRECTION_IN)
		cmd->rcvd_size = dbuf_size - residue;

	return retval;
}

/** Perform bulk-only mass storage reset.
 *
 * @param mfun		Mass storage function
 * @return		Error code
 */
int usb_massstor_reset(usbmast_dev_t *mdev)
{
	return usb_control_request_set(&mdev->usb_dev->ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    0xFF, 0, mdev->usb_dev->interface_no, NULL, 0);
}

/** Perform complete reset recovery of bulk-only mass storage.
 *
 * Notice that no error is reported because if this fails, the error
 * would reappear on next transaction somehow.
 *
 * @param mfun		Mass storage function
 */
void usb_massstor_reset_recovery(usbmast_dev_t *mdev)
{
	/* We would ignore errors here because if this fails
	 * we are doomed anyway and any following transaction would fail.
	 */
	usb_massstor_reset(mdev);
	usb_pipe_clear_halt(&mdev->usb_dev->ctrl_pipe,
	    &mdev->usb_dev->pipes[BULK_IN_EP].pipe);
	usb_pipe_clear_halt(&mdev->usb_dev->ctrl_pipe,
	    &mdev->usb_dev->pipes[BULK_OUT_EP].pipe);
}

/** Get max LUN of a mass storage device.
 *
 * @see usb_masstor_get_lun_count
 *
 * @warning Error from this command does not necessarily indicate malfunction
 * of the device. Device does not need to support this request.
 * You shall rather use usb_masstor_get_lun_count.
 *
 * @param mfun		Mass storage function
 * @return		Error code of maximum LUN (index, not count)
 */
int usb_massstor_get_max_lun(usbmast_dev_t *mdev)
{
	uint8_t max_lun;
	size_t data_recv_len;
	int rc = usb_control_request_get(&mdev->usb_dev->ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    0xFE, 0, mdev->usb_dev->interface_no, &max_lun, 1, &data_recv_len);
	if (rc != EOK) {
		return rc;
	}
	if (data_recv_len != 1) {
		return EEMPTY;
	}
	return (int) max_lun;
}

/** Get number of LUNs supported by mass storage device.
 *
 * @warning This function hides any error during the request
 * (typically that shall not be a problem).
 *
 * @param mfun		Mass storage function
 * @return		Number of LUNs
 */
size_t usb_masstor_get_lun_count(usbmast_dev_t *mdev)
{
	int max_lun = usb_massstor_get_max_lun(mdev);
	if (max_lun < 0) {
		max_lun = 1;
	} else {
		max_lun++;
	}

	return (size_t) max_lun;
}

/**
 * @}
 */
