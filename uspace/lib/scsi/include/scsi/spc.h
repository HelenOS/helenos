/*
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

/** @addtogroup libscsi
 * @{
 */
/**
 * @file SCSI Primary Commands.
 */

#ifndef LIBSCSI_SPC_H_
#define LIBSCSI_SPC_H_

#include <stdint.h>

/** SCSI command codes defined in SCSI-SPC */
enum scsi_cmd_spc {
	SCSI_CMD_INQUIRY	= 0x12
};

/** SCSI Inquiry command */
typedef struct {
	/** Operation code (12h, SCSI_CMD_INQUIRY) */
	uint8_t op_code;
	/** Reserved:7-2, obsolete:1, evpd:0 */
	uint8_t evpd;
	/* Page Code */
	uint8_t page_code;
	/* Allocation Length */
	uint16_t alloc_len;
	/* Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_inquiry_t;

/** Minimum size of inquiry data required since SCSI-2 */
#define SCSI_STD_INQUIRY_DATA_MIN_SIZE 36

/** Standard inquiry data.
 *
 * Returned for Inquiry command with evpd bit cleared.
 */
typedef struct {
	/** Peripheral qualifier, Peripheral device type */
	uint8_t pqual_devtype;
	/** RMB, reserved */
	uint8_t rmb;
	/** Version */
	uint8_t version;
	/** Obsolete, NormACA, HiSup, Response Data Format */
	uint8_t aca_hisup_rdf;
	/** Additional Length */
	uint8_t additional_len;
	/** SCCS, ACC, TPGS, 3PC, Reserved, Protect */
	uint8_t cap1;
	/** Obsolete, EncServ, VS, MuliP, Obsolete, Addr16 */
	uint8_t cap2;
	/** Obsolete, WBus16, Sync, Obsolete, CmdQue, VS */
	uint8_t cap3;

	/** Vendor string */
	uint8_t vendor[8];
	/** Product string */
	uint8_t product[16];
	/** Revision string */
	uint8_t revision[4];

	/* End of required data */
} scsi_std_inquiry_data_t;

/** Bits in scsi_std_inquiry_data_t.pqual_devtype */
enum scsi_pqual_devtype_bits {
	/* Peripheral qualifier */
	SCSI_PQDT_PQUAL_h	= 7,
	SCSI_PQDT_PQUAL_l	= 5,

	/* Peripheral device type */
	SCSI_PQDT_DEV_TYPE_h	= 4,
	SCSI_PQDT_DEV_TYPE_l	= 0
};

/** Bits in scsi_std_inquiry_data_t.rmb */
enum scsi_rmb_bits {
	SCSI_RMB_RMB		= 7
};

/** SCSI peripheral device type */
enum scsi_device_type {
	SCSI_DEV_BLOCK		= 0x00,
	SCSI_DEV_STREAM		= 0x01,
	SCSI_DEV_CD_DVD		= 0x05,
	SCSI_DEV_CHANGER	= 0x08,
	SCSI_DEV_ENCLOSURE	= 0x0d,
	SCSI_DEV_OSD		= 0x11,

	SCSI_DEV_LIMIT		= 0x20
};

extern const char *scsi_dev_type_str[SCSI_DEV_LIMIT];
extern const char *scsi_get_dev_type_str(unsigned);

#endif

/** @}
 */
