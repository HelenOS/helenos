/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef TYPES_DISPLAY_PTD_EVENT_H
#define TYPES_DISPLAY_PTD_EVENT_H

#include <gfx/coord.h>

typedef enum {
	PTD_MOVE,
	PTD_ABS_MOVE,
	PTD_PRESS,
	PTD_RELEASE,
	PTD_DCLICK
} ptd_event_type_t;

/** Pointing device event */
typedef struct {
	ptd_event_type_t type;
	/** Button number for PTD_PRESS or PTD_RELEASE */
	int btn_num;
	/** Relative move vector for PTD_MOVE */
	gfx_coord2_t dmove;
	/** Absolute position for PTD_ABS_MOVE */
	gfx_coord2_t apos;
	/** Absolute position bounds for PTD_ABS_MOVE */
	gfx_rect_t abounds;
} ptd_event_t;

#endif

/** @}
 */
