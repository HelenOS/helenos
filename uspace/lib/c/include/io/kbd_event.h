/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_KBD_EVENT_H_
#define _LIBC_IO_KBD_EVENT_H_

#include <adt/list.h>
#include <inttypes.h>
#include <io/keycode.h>

typedef enum {
	KEY_PRESS,
	KEY_RELEASE
} kbd_event_type_t;

/** Console event structure. */
typedef struct {
	/** List handle */
	link_t link;

	/** Press or release event. */
	kbd_event_type_t type;

	/** Keycode of the key that was pressed or released. */
	keycode_t key;

	/** Bitmask of modifiers held. */
	keymod_t mods;

	/** The character that was generated or '\0' for none. */
	char32_t c;
} kbd_event_t;

#endif

/** @}
 */
