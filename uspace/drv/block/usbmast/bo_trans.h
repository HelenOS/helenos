/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbmast
 * @{
 */
/** @file
 * USB mass storage bulk-only transport.
 */

#ifndef BO_TRANS_H_
#define BO_TRANS_H_

#include <scsi/spc.h>
#include <stddef.h>
#include <stdint.h>
#include <usb/usb.h>
#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>
#include "usbmast.h"

typedef enum cmd_status {
	CMDS_GOOD,
	CMDS_FAILED
} cmd_status_t;

/** SCSI command.
 *
 * Contains (a subset of) the input and output arguments of SCSI
 * Execute Command procedure call (see SAM-4 chapter 5.1)
 */
typedef struct {
	/*
	 * Related to IN fields
	 */

	/** Command Descriptor Block */
	const void *cdb;
	/** CDB size in bytes */
	size_t cdb_size;

	/** Outgoing data */
	const void *data_out;
	/** Size of outgoing data in bytes */
	size_t data_out_size;

	/*
	 * Related to OUT fields
	 */

	/** Buffer for incoming data */
	void *data_in;
	/** Size of input buffer in bytes */
	size_t data_in_size;

	/** Number of bytes actually received */
	size_t rcvd_size;

	/** Status */
	cmd_status_t status;
} scsi_cmd_t;

extern errno_t usb_massstor_cmd(usbmast_fun_t *, uint32_t, scsi_cmd_t *);
extern errno_t usb_massstor_reset(usbmast_dev_t *);
extern void usb_massstor_reset_recovery(usbmast_dev_t *);
extern int usb_massstor_get_max_lun(usbmast_dev_t *);
extern size_t usb_masstor_get_lun_count(usbmast_dev_t *);

#endif

/**
 * @}
 */
