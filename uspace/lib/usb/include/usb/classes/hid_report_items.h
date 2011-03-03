/*
 * Copyright (c) 2010 Vojtech Horky
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
 * @brief USB HID parser.
 */
#ifndef LIBUSB_HID_REPORT_ITEMS_H_
#define LIBUSB_HID_REPORT_ITEMS_H_

#include <stdint.h>

/* MAIN ITEMS */
#define USB_HID_TAG_CLASS_MAIN				0x0
#define USB_HID_REPORT_TAG_INPUT			0x8
#define USB_HID_REPORT_TAG_OUTPUT			0x9
#define USB_HID_REPORT_TAG_FEATURE			0xA
#define USB_HID_REPORT_TAG_COLLECTION		0xB
#define USB_HID_REPORT_TAG_END_COLLECTION	0xC

/* GLOBAL ITEMS */
#define USB_HID_TAG_CLASS_GLOBAL			0x1
#define USB_HID_REPORT_TAG_USAGE_PAGE		0x0
#define USB_HID_REPORT_TAG_LOGICAL_MINIMUM	0x1
#define USB_HID_REPORT_TAG_LOGICAL_MAXIMUM	0x2
#define USB_HID_REPORT_TAG_PHYSICAL_MINIMUM 0x3
#define USB_HID_REPORT_TAG_PHYSICAL_MAXIMUM 0x4
#define USB_HID_REPORT_TAG_UNIT_EXPONENT	0x5
#define USB_HID_REPORT_TAG_UNIT				0x6
#define USB_HID_REPORT_TAG_REPORT_SIZE		0x7
#define USB_HID_REPORT_TAG_REPORT_ID		0x8
#define USB_HID_REPORT_TAG_REPORT_COUNT		0x9
#define USB_HID_REPORT_TAG_PUSH				0xA
#define USB_HID_REPORT_TAG_POP				0xB


/* LOCAL ITEMS */
#define USB_HID_TAG_CLASS_LOCAL					0x2
#define USB_HID_REPORT_TAG_USAGE				0x0
#define USB_HID_REPORT_TAG_USAGE_MINIMUM		0x1
#define USB_HID_REPORT_TAG_USAGE_MAXIMUM		0x2
#define USB_HID_REPORT_TAG_DESIGNATOR_INDEX		0x3
#define USB_HID_REPORT_TAG_DESIGNATOR_MINIMUM	0x4
#define USB_HID_REPORT_TAG_DESIGNATOR_MAXIMUM	0x5
#define USB_HID_REPORT_TAG_STRING_INDEX			0x7
#define USB_HID_REPORT_TAG_STRING_MINIMUM		0x8
#define USB_HID_REPORT_TAG_STRING_MAXIMUM		0x9
#define USB_HID_REPORT_TAG_DELIMITER			0xA

#endif
/**
 * @}
 */
