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

/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief USB HID Report descriptor item tags.
 */
#ifndef LIBUSB_HID_REPORT_ITEMS_H_
#define LIBUSB_HID_REPORT_ITEMS_H_

#include <stdint.h>

/*
 * Item prefix
 */

/** Returns size of item data in bytes */
#define USB_HID_ITEM_SIZE(data) 	((uint8_t)(data & 0x3))

/** Returns item tag */
#define USB_HID_ITEM_TAG(data) 		((uint8_t)((data & 0xF0) >> 4))

/** Returns class of item tag */
#define USB_HID_ITEM_TAG_CLASS(data)	((uint8_t)((data & 0xC) >> 2))

/** Returns if the item is the short item or long item. Long items are not
 * supported.
 */
#define USB_HID_ITEM_IS_LONG(data)	(data == 0xFE)

/*
 * Extended usage macros
 */

/** Recognizes if the given usage is extended (contains also usage page).  */
#define USB_HID_IS_EXTENDED_USAGE(usage)	((usage & 0xFFFF0000) != 0)

/** Cuts usage page of the extended usage. */
#define USB_HID_EXTENDED_USAGE_PAGE(usage)	((usage & 0xFFFF0000) >> 16)

/** Cuts usage of the extended usage */
#define USB_HID_EXTENDED_USAGE(usage)		(usage & 0xFFFF)

/*
 * Input/Output/Feature Item flags
 */
/**
 * Indicates whether the item is data (0) or a constant (1) value. Data
 * indicates the item is defining report fields that contain modifiable device
 * data. Constant indicates the item is a static read-only field in a report
 * and cannot be modified (written) by the host.
 */
#define USB_HID_ITEM_FLAG_CONSTANT(flags) 	((flags & 0x1) == 0x1)

/**
 * Indicates whether the item creates variable (1) or array (0) data fields in
 * reports.
 */
#define USB_HID_ITEM_FLAG_VARIABLE(flags) 	((flags & 0x2) == 0x2)

/**
 * Indicates whether the data is absolute (0) (based on a fixed origin) or
 * relative (1) (indicating the change in value from the last report). Mouse
 * devices usually provide relative data, while tablets usually provide
 * absolute data.
 */
#define USB_HID_ITEM_FLAG_RELATIVE(flags) 	((flags & 0x4) == 0x4)

/** Indicates whether the data “rolls over” when reaching either the extreme
 * high or low value. For example, a dial that can spin freely 360 degrees
 * might output values from 0 to 10. If Wrap is indicated, the next value
 * reported after passing the 10 position in the increasing direction would be
 * 0.
 */
#define USB_HID_ITEM_FLAG_WRAP(flags)		((flags & 0x8) == 0x8)

/**
 * Indicates whether the raw data from the device has been processed in some
 * way, and no longer represents a linear relationship between what is
 * measured and the data that is reported.
 */
#define USB_HID_ITEM_FLAG_LINEAR(flags)		((flags & 0x10) == 0x10)

/**
 * Indicates whether the control has a preferred state to which it will return
 * when the user is not physically interacting with the control. Push buttons
 * (as opposed to toggle buttons) and self- centering joysticks are examples.
 */
#define USB_HID_ITEM_FLAG_PREFERRED(flags)	((flags & 0x20) == 0x20)

/**
 * Indicates whether the control has a state in which it is not sending
 * meaningful data. One possible use of the null state is for controls that
 * require the user to physically interact with the control in order for it to
 * report useful data.
 */
#define USB_HID_ITEM_FLAG_POSITION(flags)	((flags & 0x40) == 0x40)

/**
 * Indicates whether the Feature or Output control's value should be changed
 * by the host or not.  Volatile output can change with or without host
 * interaction. To avoid synchronization problems, volatile controls should be
 * relative whenever possible.
 */
#define USB_HID_ITEM_FLAG_VOLATILE(flags)	((flags & 0x80) == 0x80)

/**
 * Indicates that the control emits a fixed-size stream of bytes. The contents
 * of the data field are determined by the application. The contents of the
 * buffer are not interpreted as a single numeric quantity. Report data
 * defined by a Buffered Bytes item must be aligned on an 8-bit boundary.
 */
#define USB_HID_ITEM_FLAG_BUFFERED(flags)	((flags & 0x100) == 0x100)

/* MAIN ITEMS */

/**
 * Main items are used to either define or group certain types of data fields
 * within a Report descriptor.
 */
#define USB_HID_TAG_CLASS_MAIN			0x0

/**
 * An Input item describes information about the data provided by one or more
 * physical controls. An application can use this information to interpret the
 * data provided by the device. All data fields defined in a single item share
 * an identical data format.
 */
#define USB_HID_REPORT_TAG_INPUT		0x8

/**
 * The Output item is used to define an output data field in a report. This
 * item is similar to an Input item except it describes data sent to the
 * device—for example, LED states.
 */
#define USB_HID_REPORT_TAG_OUTPUT		0x9

/**
 * Feature items describe device configuration information that can be sent to
 * the device.
 */
#define USB_HID_REPORT_TAG_FEATURE		0xB

/**
 * A Collection item identifies a relationship between two or more data
 * (Input, Output, or Feature.)
 */
#define USB_HID_REPORT_TAG_COLLECTION		0xA

/**
 * While the Collection item opens a collection of data, the End Collection
 * item closes a collection.
 */
#define USB_HID_REPORT_TAG_END_COLLECTION	0xC

/* GLOBAL ITEMS */

/**
 * Global items describe rather than define data from a control.
 */
#define USB_HID_TAG_CLASS_GLOBAL		0x1

/**
 * Unsigned integer specifying the current Usage Page. Since a usage are 32
 * bit values, Usage Page items can be used to conserve space in a report
 * descriptor by setting the high order 16 bits of a subsequent usages. Any
 * usage that follows which is defines 16 bits or less is interpreted as a
 * Usage ID and concatenated with the Usage Page to form a 32 bit Usage.
 */
#define USB_HID_REPORT_TAG_USAGE_PAGE		0x0

/**
 * Extent value in logical units. This is the minimum value that a variable
 * or array item will report. For example, a mouse reporting x position values
 * from 0 to 128 would have a Logical Minimum of 0 and a Logical Maximum of
 * 128.
 */
#define USB_HID_REPORT_TAG_LOGICAL_MINIMUM	0x1

/**
 * Extent value in logical units. This is the maximum value that a variable
 * or array item will report.
 */
#define USB_HID_REPORT_TAG_LOGICAL_MAXIMUM	0x2

/**
 * Minimum value for the physical extent of a variable item. This represents
 * the Logical Minimum with units applied to it.
 */
#define USB_HID_REPORT_TAG_PHYSICAL_MINIMUM 	0x3

/**
 * Maximum value for the physical extent of a variable item.
 */
#define USB_HID_REPORT_TAG_PHYSICAL_MAXIMUM 	0x4

/**
 * Value of the unit exponent in base 10. See the table later in this section
 * for more information.
 */
#define USB_HID_REPORT_TAG_UNIT_EXPONENT	0x5

/**
 * Unit values.
 */
#define USB_HID_REPORT_TAG_UNIT			0x6

/**
 * Unsigned integer specifying the size of the report fields in bits. This
 * allows the parser to build an item map for the report handler to use.
 */
#define USB_HID_REPORT_TAG_REPORT_SIZE		0x7

/**
 * Unsigned value that specifies the Report ID. If a Report ID tag is used
 * anywhere in Report descriptor, all data reports for the device are preceded
 * by a single byte ID field. All items succeeding the first Report ID tag but
 * preceding a second Report ID tag are included in a report prefixed by a
 * 1-byte ID. All items succeeding the second but preceding a third Report ID
 * tag are included in a second report prefixed by a second ID, and so on.
 */
#define USB_HID_REPORT_TAG_REPORT_ID		0x8

/**
 * Unsigned integer specifying the number of data fields for the item;
 * determines how many fields are included in the report for this particular
 * item (and consequently how many bits are added to the report).
 */
#define USB_HID_REPORT_TAG_REPORT_COUNT		0x9

/**
 * Places a copy of the global item state table on the stack.
 */
#define USB_HID_REPORT_TAG_PUSH			0xA

/**
 * Replaces the item state table with the top structure from the stack.
 */
#define USB_HID_REPORT_TAG_POP			0xB

/* LOCAL ITEMS */

/**
 * Local item tags define characteristics of controls. These items do not
 * carry over to the next Main item. If a Main item defines more than one
 * control, it may be preceded by several similar Local item tags. For
 * example, an Input item may have several Usage tags associated with it, one
 * for each control.
 */
#define USB_HID_TAG_CLASS_LOCAL			0x2

/**
 * Usage index for an item usage; represents a suggested usage for the item or
 * collection. In the case where an item represents multiple controls, a Usage
 * tag may suggest a usage for every variable or element in an array.
 */
#define USB_HID_REPORT_TAG_USAGE		0x0

/**
 * Defines the starting usage associated with an array or bitmap.
 */
#define USB_HID_REPORT_TAG_USAGE_MINIMUM	0x1

/**
 * Defines the ending usage associated with an array or bitmap.
 */
#define USB_HID_REPORT_TAG_USAGE_MAXIMUM	0x2

/**
 * Determines the body part used for a control. Index points to a designator
 * in the Physical descriptor.
 */
#define USB_HID_REPORT_TAG_DESIGNATOR_INDEX	0x3

/**
 * Defines the index of the starting designator associated with an array or
 * bitmap.
 */
#define USB_HID_REPORT_TAG_DESIGNATOR_MINIMUM	0x4

/**
 * Defines the index of the ending designator associated with an array or
 * bitmap.
 */
#define USB_HID_REPORT_TAG_DESIGNATOR_MAXIMUM	0x5

/**
 * String index for a String descriptor; allows a string to be associated with
 * a particular item or control.
 */
#define USB_HID_REPORT_TAG_STRING_INDEX		0x7

/**
 * Specifies the first string index when assigning a group of sequential
 * strings to controls in an array or bitmap.
 */
#define USB_HID_REPORT_TAG_STRING_MINIMUM	0x8

/**
 * Specifies the last string index when assigning a group of sequential
 * strings to controls in an array or bitmap.
 */
#define USB_HID_REPORT_TAG_STRING_MAXIMUM	0x9

/**
 * Defines the beginning or end of a set of local items (1 = open set, 0 =
 * close set).
 *
 * Usages other than the first (most preferred) usage defined are not
 * accessible by system software.
 */
#define USB_HID_REPORT_TAG_DELIMITER		0xA

#endif
/**
 * @}
 */
