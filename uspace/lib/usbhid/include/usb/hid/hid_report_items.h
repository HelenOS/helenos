/*
 * Copyright (c) 2011 Matej Klonfar
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

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * @brief USB HID parser.
 */
#ifndef LIBUSBHID_HID_REPORT_ITEMS_H_
#define LIBUSBHID_HID_REPORT_ITEMS_H_

#include <stdint.h>

/**
 * Item prefix
 */
#define USB_HID_ITEM_SIZE(data) 	((uint8_t)(data & 0x3))
#define USB_HID_ITEM_TAG(data) 		((uint8_t)((data & 0xF0) >> 4))
#define USB_HID_ITEM_TAG_CLASS(data)	((uint8_t)((data & 0xC) >> 2))
#define USB_HID_ITEM_IS_LONG(data)	(data == 0xFE)


/**
 * Extended usage macros
 */
#define USB_HID_IS_EXTENDED_USAGE(usage)	((usage & 0xFFFF0000) != 0)
#define USB_HID_EXTENDED_USAGE_PAGE(usage)	((usage & 0xFFFF0000) >> 16)
#define USB_HID_EXTENDED_USAGE(usage)		(usage & 0xFFFF)

/**
 * Input/Output/Feature Item flags
 */
/** Constant (1) / Variable (0) */
#define USB_HID_ITEM_FLAG_CONSTANT(flags) 	((flags & 0x1) == 0x1)
/** Variable (1) / Array (0) */
#define USB_HID_ITEM_FLAG_VARIABLE(flags) 	((flags & 0x2) == 0x2)
/** Absolute / Relative*/
#define USB_HID_ITEM_FLAG_RELATIVE(flags) 	((flags & 0x4) == 0x4)
/** Wrap / No Wrap */
#define USB_HID_ITEM_FLAG_WRAP(flags)		((flags & 0x8) == 0x8)
#define USB_HID_ITEM_FLAG_LINEAR(flags)		((flags & 0x10) == 0x10)
#define USB_HID_ITEM_FLAG_PREFERRED(flags)	((flags & 0x20) == 0x20)
#define USB_HID_ITEM_FLAG_POSITION(flags)	((flags & 0x40) == 0x40)
#define USB_HID_ITEM_FLAG_VOLATILE(flags)	((flags & 0x80) == 0x80)
#define USB_HID_ITEM_FLAG_BUFFERED(flags)	((flags & 0x100) == 0x100)

/* MAIN ITEMS */
#define USB_HID_TAG_CLASS_MAIN				0x0
#define USB_HID_REPORT_TAG_INPUT			0x8
#define USB_HID_REPORT_TAG_OUTPUT			0x9
#define USB_HID_REPORT_TAG_FEATURE			0xB
#define USB_HID_REPORT_TAG_COLLECTION		0xA
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
