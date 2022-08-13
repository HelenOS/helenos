/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbvirtkbd
 * @{
 */
/** @file
 * @brief HID Report Descriptor.
 */
#ifndef VUK_REPORT_H_
#define VUK_REPORT_H_

#include "items.h"

/** Use standard Usage Page. */
#define STD_USAGE_PAGE(page) \
	ITEM_CREATE1(ITEM_GLOBAL, TAG_USAGE_PAGE, page)

/** Usage with one byte usage ID. */
#define USAGE1(usage_id) \
	ITEM_CREATE1(ITEM_LOCAL, TAG_USAGE, usage_id)

/** Start a collection. */
#define START_COLLECTION(collection) \
	ITEM_CREATE1(ITEM_MAIN, TAG_COLLECTION, collection)

/** End a collection. */
#define END_COLLECTION() \
	ITEM_CREATE0(ITEM_MAIN, TAG_END_COLLECTION)

#define USAGE_MINIMUM1(value) \
	ITEM_CREATE1(ITEM_LOCAL, TAG_USAGE_MINIMUM, value)

#define USAGE_MAXIMUM1(value) \
	ITEM_CREATE1(ITEM_LOCAL, TAG_USAGE_MAXIMUM, value)

#define LOGICAL_MINIMUM1(value) \
	ITEM_CREATE1(ITEM_GLOBAL, TAG_LOGICAL_MINIMUM, value)

#define LOGICAL_MAXIMUM1(value) \
	ITEM_CREATE1(ITEM_GLOBAL, TAG_LOGICAL_MAXIMUM, value)

#define REPORT_SIZE1(size) \
	ITEM_CREATE1(ITEM_GLOBAL, TAG_REPORT_SIZE, size)

#define REPORT_COUNT1(count) \
	ITEM_CREATE1(ITEM_GLOBAL, TAG_REPORT_COUNT, count)

#define INPUT(modifiers) \
	ITEM_CREATE1(ITEM_MAIN, TAG_INPUT, modifiers)

#define OUTPUT(modifiers) \
	ITEM_CREATE1(ITEM_MAIN, TAG_OUTPUT, modifiers)

#endif
/**
 * @}
 */
