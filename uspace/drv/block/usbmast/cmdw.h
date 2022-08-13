/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbmast
 * @{
 */
/** @file
 * USB mass storage commands.
 */

#ifndef CMDW_H_
#define CMDW_H_

#include <stdint.h>
#include <usb/usb.h>

typedef struct {
	uint32_t dCBWSignature;
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWBLength;
	uint8_t CBWCB[16];
} __attribute__((packed)) usb_massstor_cbw_t;

typedef struct {
	uint32_t dCSWSignature;
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t dCSWStatus;
} __attribute__((packed)) usb_massstor_csw_t;

enum cmd_block_status {
	cbs_passed	= 0x00,
	cbs_failed	= 0x01,
	cbs_phase_error = 0x02
};

extern void usb_massstor_cbw_prepare(usb_massstor_cbw_t *, uint32_t, uint32_t,
    usb_direction_t, uint8_t, uint8_t, const uint8_t *);

#endif

/**
 * @}
 */
