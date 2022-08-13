/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Event
 */

#ifndef _UI_TYPES_EVENT_H
#define _UI_TYPES_EVENT_H

/** Event claim.
 *
 * This type is returned by an event processing function to signal whether
 * it ruled that the event is destined for it or not
 */
typedef enum {
	/** Event claimed */
	ui_claimed,
	/** Event not claimed */
	ui_unclaimed
} ui_evclaim_t;

#endif

/** @}
 */
