/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup trace
 * @{
 */
/** @file
 */

#ifndef TRACE_H_
#define TRACE_H_

#include <types/common.h>

/**
 * Classes of events that can be displayed. Can be or-ed together.
 */
typedef enum {

	DM_THREAD	= 1,	/**< Thread creation and termination events */
	DM_SYSCALL	= 2,	/**< System calls */
	DM_IPC		= 4,	/**< Low-level IPC */
	DM_SYSTEM	= 8,	/**< Sysipc protocol */
	DM_USER		= 16	/**< User IPC protocols */

} display_mask_t;

typedef enum {
	V_VOID,
	V_INTEGER,
	V_PTR,
	V_HASH,
	V_ERRNO,
	V_INT_ERRNO,
	V_CHAR
} val_type_t;

/** Combination of events to print. */
extern display_mask_t display_mask;

void val_print(sysarg_t val, val_type_t v_type);

#endif

/** @}
 */
