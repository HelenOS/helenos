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
