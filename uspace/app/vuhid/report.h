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
