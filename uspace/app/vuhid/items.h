/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbvirtkbd
 * @{
 */
/** @file
 * @brief HID Item related functions.
 */
#ifndef VUK_ITEM_H_
#define VUK_ITEM_H_

#include <stdint.h>

typedef uint8_t report_descriptor_data_t[];

/* Item types. */
#define ITEM_MAIN 0
#define ITEM_GLOBAL 1
#define ITEM_LOCAL 2

/* Item tags. */

/* Main item tags. */
#define TAG_INPUT 8
#define TAG_OUTPUT 9
#define TAG_FEATURE 11
#define TAG_COLLECTION 10
#define TAG_END_COLLECTION 12

/* Global item tags. */
#define TAG_USAGE_PAGE 0
#define TAG_LOGICAL_MINIMUM 1
#define TAG_LOGICAL_MAXIMUM 2
#define TAG_REPORT_SIZE 7
#define TAG_REPORT_COUNT 9

/* Local item tags. */
#define TAG_USAGE 0
#define TAG_USAGE_MINIMUM 1
#define TAG_USAGE_MAXIMUM 2

/* Bits for Input, Output and Feature items. */
#define _IOF(value, shift) ((value) << (shift))
#define IOF_DATA _IOF(0, 0)
#define IOF_CONSTANT _IOF(1, 0)
#define IOF_ARRAY _IOF(0, 1)
#define IOF_VARIABLE _IOF(1, 1)
#define IOF_ABSOLUTE _IOF(0, 2)
#define IOF_RELATIVE _IOF(1, 2)
/* ... */

/* Collection types. */
#define COLLECTION_PHYSICAL 0x00
#define COLLECTION_APPLICATION 0x01

/** Creates item prefix.
 * @param size Item size.
 * @param type Item type.
 * @param tag Item tag.
 */
#define BUILD_ITEM_PREFIX(size, type, tag) \
	((size) | ((type) << 2) | ((tag) << 4))

/** Create no-data item.
 * @param type Item type.
 * @param tag Item tag.
 */
#define ITEM_CREATE0(type, tag) \
	BUILD_ITEM_PREFIX(0, type, tag)

/** Create item with 1-byte data.
 * @param type Item type.
 * @param tag Item tag.
 * @param data Item data (single byte).
 */
#define ITEM_CREATE1(type, tag, data) \
	BUILD_ITEM_PREFIX(1, type, tag), data

#endif
/**
 * @}
 */
