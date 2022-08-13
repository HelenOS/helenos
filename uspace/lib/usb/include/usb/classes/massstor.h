/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB mass storage related functions and constants.
 */
#ifndef LIBUSB_CLASS_MASSSTOR_H_
#define LIBUSB_CLASS_MASSSTOR_H_

/** USB mass storage subclasses. */
typedef enum {
	USB_MASSSTOR_SUBCLASS_RBC = 0x01,
	/** Also known as MMC-5. */
	USB_MASSSTOR_SUBCLASS_ATAPI = 0x02,
	USB_MASSSTOR_SUBCLASS_UFI = 0x04,
	USB_MASSSTOR_SUBCLASS_SCSI = 0x06,
	USB_MASSSTOR_SUBCLASS_LSDFS = 0x07,
	USB_MASSSTOR_SUBCLASS_IEEE1667 = 0x08,
	USB_MASSSTOR_SUBCLASS_VENDOR = 0xFF
} usb_massstor_subclass_t;

/** USB mass storage interface protocols. */
typedef enum {
	/** CBI transport with command completion interrupt. */
	USB_MASSSTOR_PROTOCOL_CBI_CC = 0x00,
	/** CBI transport with no command completion interrupt. */
	USB_MASSSTOR_PROTOCOL_CBI = 0x01,
	/** Bulk only transport. */
	USB_MASSSTOR_PROTOCOL_BBB = 0x50,
	/** USB attached SCSI. */
	USB_MASSSTOR_PROTOCOL_UAS = 0x62,
	USB_MASSSTOR_PROTOCOL_VENDOR = 0xFF
} usb_massstor_protocol_t;

#endif
/**
 * @}
 */
