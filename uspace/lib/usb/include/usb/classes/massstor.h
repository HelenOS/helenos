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
