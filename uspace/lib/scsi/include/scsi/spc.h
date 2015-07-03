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
#include <str.h>

/** SCSI command codes defined in SCSI-SPC */
enum scsi_cmd_spc {
	SCSI_CMD_TEST_UNIT_READY = 0x00,
	SCSI_CMD_REQUEST_SENSE	 = 0x03,
	SCSI_CMD_INQUIRY	 = 0x12,
};

/** SCSI Inquiry command
 *
 * Note: for SFF 8020 the command must be zero-padded to 12 bytes
 * and alloc_len must be <= 0xff.
 */
typedef struct {
	/** Operation code (SCSI_CMD_INQUIRY) */
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
} __attribute__((packed)) scsi_std_inquiry_data_t;

/** Size of struct or union member. */
#define SCSI_MEMBER_SIZE(type, member) \
    (sizeof(((type *)0) -> member))

/** Size of string buffer needed to hold converted inquiry vendor string */
#define SCSI_INQ_VENDOR_STR_BUFSIZE \
    SPASCII_STR_BUFSIZE(SCSI_MEMBER_SIZE(scsi_std_inquiry_data_t, vendor))

/** Size of string buffer needed to hold converted inquiry product string */
#define SCSI_INQ_PRODUCT_STR_BUFSIZE \
    SPASCII_STR_BUFSIZE(SCSI_MEMBER_SIZE(scsi_std_inquiry_data_t, product))

/** Size of string buffer needed to hold converted inquiry revision string */
#define SCSI_INQ_REVISION_STR_BUFSIZE \
    SPASCII_STR_BUFSIZE(SCSI_MEMBER_SIZE(scsi_std_inquiry_data_t, revision))

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

/** SCSI Request Sense command */
typedef struct {
	/** Operation code (SCSI_CMD_REQUEST_SENSE) */
	uint8_t op_code;
	/** Reserved, Desc */
	uint8_t desc;
	/* Reserved */
	uint16_t res_2;
	/* Allocation Length */
	uint8_t alloc_len;
	/* Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_request_sense_t;

/** Minimum size of sense data.
 *
 * If the target returns less data, the missing bytes should be considered
 * zero.
 */
#define SCSI_SENSE_DATA_MIN_SIZE 18

/** Maximum size of sense data */
#define SCSI_SENSE_DATA_MAX_SIZE 252

/** Fixed-format sense data.
 *
 * Returned for Request Sense command with Desc bit cleared.
 */
typedef struct {
	/** Valid, Response Code */
	uint8_t valid_rcode;
	/** Peripheral qualifier, Peripheral device type */
	uint8_t obsolete_1;
	/** Filemark, EOM, ILI, Reserved, Sense Key */
	uint8_t flags_key;
	/** Information */
	uint32_t info;
	/** Additional Sense Length */
	uint8_t additional_len;
	/** Command-specific Information */
	uint32_t cmd_spec;
	/** Additional Sense Code */
	uint8_t additional_code;
	/** Additional Sense Code Qualifier */
	uint8_t additional_cqual;
	/** Field-replaceable Unit Code */
	uint8_t fru_code;
	/** SKSV, Sense-key specific */
	uint8_t key_spec[3];
} __attribute__((packed)) scsi_sense_data_t;

/** Sense key */
enum scsi_sense_key {
	SCSI_SK_NO_SENSE	= 0x0,
	SCSI_SK_RECOVERED_ERROR	= 0x1,
	SCSI_SK_NOT_READY	= 0x2,
	SCSI_SK_MEDIUM_ERROR	= 0x3,
	SCSI_SK_HARDWARE_ERROR	= 0x4,
	SCSI_SK_ILLEGAL_REQUEST	= 0x5,
	SCSI_SK_UNIT_ATTENTION	= 0x6,
	SCSI_SK_DATA_PROTECT	= 0x7,
	SCSI_SK_BLANK_CHECK	= 0x8,
	SCSI_SK_VENDOR_SPECIFIC	= 0x9,
	SCSI_SK_COPY_ABORTED	= 0xa,
	SCSI_SK_ABORTED_COMMAND	= 0xb,
	SCSI_SK_VOLUME_OVERFLOW	= 0xd,
	SCSI_SK_MISCOMPARE	= 0xe,

	SCSI_SK_LIMIT		= 0x10
};

extern const char *scsi_dev_type_str[SCSI_DEV_LIMIT];
extern const char *scsi_sense_key_str[SCSI_SK_LIMIT];

extern const char *scsi_get_dev_type_str(unsigned);
extern const char *scsi_get_sense_key_str(unsigned);

/** SCSI Test Unit Ready command */
typedef struct {
	/** Operation code (SCSI_CMD_TEST_UNIT_READY) */
	uint8_t op_code;
	/** Reserved */
	uint32_t reserved;
	/* Control */
	uint8_t control;
} __attribute__((packed)) scsi_cdb_test_unit_ready_t;

#endif

/** @}
 */
