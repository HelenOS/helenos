/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_CONS_EVENT_H_
#define _LIBC_IO_CONS_EVENT_H_

#include <adt/list.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>

typedef enum {
	/** Key event */
	CEV_KEY,
	/** Position event */
	CEV_POS
} cons_event_type_t;

/** Console event structure. */
typedef struct {
	/** List handle */
	link_t link;

	/** Event type */
	cons_event_type_t type;

	union {
		kbd_event_t key;
		pos_event_t pos;
	} ev;
} cons_event_t;

#endif

/** @}
 */
