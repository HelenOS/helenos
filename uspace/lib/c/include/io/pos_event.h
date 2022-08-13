/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2012 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_POS_EVENT_H_
#define _LIBC_IO_POS_EVENT_H_

#include <types/common.h>

typedef enum {
	/** Position update */
	POS_UPDATE,
	/** Button press */
	POS_PRESS,
	/** Button release */
	POS_RELEASE,
	/** Double click */
	POS_DCLICK
} pos_event_type_t;

/** Positioning device event */
typedef struct {
	sysarg_t pos_id;
	pos_event_type_t type;
	sysarg_t btn_num;
	sysarg_t hpos;
	sysarg_t vpos;
} pos_event_t;

#endif

/** @}
 */
