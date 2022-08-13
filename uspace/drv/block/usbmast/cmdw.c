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

#include <byteorder.h>
#include <mem.h>
#include <stdint.h>
#include <usb/usb.h>
#include "cmdw.h"

void usb_massstor_cbw_prepare(usb_massstor_cbw_t *cbw,
    uint32_t tag, uint32_t transfer_length, usb_direction_t dir,
    uint8_t lun, uint8_t cmd_len, const uint8_t *cmd)
{
	cbw->dCBWSignature = uint32_host2usb(0x43425355);
	cbw->dCBWTag = tag;
	cbw->dCBWDataTransferLength = transfer_length;

	cbw->bmCBWFlags = 0;
	if (dir == USB_DIRECTION_IN) {
		cbw->bmCBWFlags |= (1 << 7);
	}

	/* Only lowest 4 bits. */
	cbw->bCBWLUN = lun & 0x0F;

	/* Only lowest 5 bits. */
	cbw->bCBWBLength = cmd_len & 0x1F;

	memcpy(cbw->CBWCB, cmd, cbw->bCBWBLength);
}

/**
 * @}
 */
